#include "syntax/tree.h"
#include <cstring>
namespace cppgm {
namespace {
const char* kind_name(SyntaxKind kind) {
    switch (kind) {
#define X(name, text) case SyntaxKind::name: return text;
    CPPGM_SYNTAX_KINDS(X)
#undef X
    }
    return "?";
}
const char* token_spelling(SimpleKind kind) {
    // A bounded metadata table, never a semantic/name lookup key.
    static const char* const words[] = {
        "alignas","alignof","asm","auto","bool","break","case","catch","char","char16_t","char32_t",
        "class","const","constexpr","const_cast","continue","decltype","default","delete","do","double",
        "dynamic_cast","else","enum","explicit","export","extern","false","float","for","friend","goto",
        "if","inline","int","long","mutable","namespace","new","noexcept","nullptr","operator","private",
        "protected","public","register","reinterpret_cast","return","short","signed","sizeof","static",
        "static_assert","static_cast","struct","switch","template","this","thread_local","throw","true","try",
        "typedef","typeid","typename","union","unsigned","using","virtual","void","volatile","wchar_t","while",
        "{","}","[","]","(",")","|","^","~","&","!",";",":","...","?","::",".",".*","+","-","*","/","%","=","<",">",
        "+=","-=","*=","/=","%=","^=","&=","|=","<<",">>",">>=","<<=","==","!=","<=",">=","&&","||","++","--",",","->*","->"
    };
    return words[static_cast<unsigned>(kind)];
}
}
SyntaxTree::SyntaxTree() { SyntaxNode n; n.kind=SyntaxKind::TranslationUnit; nodes.push_back(n); edges.push_back({0,0}); }
NodeId SyntaxTree::node(SyntaxKind kind, SourceLocation location) {
    SyntaxNode n; n.kind=kind; n.location=location.line ? location : anchor;
    nodes.push_back(n); return nodes.size()-1;
}
NodeId SyntaxTree::name(SyntaxKind kind, IdentifierId id, SourceLocation location) {
    NodeId n=node(kind,location); nodes[n].payload=SyntaxPayload::identifier; nodes[n].name=id; return n;
}
NodeId SyntaxTree::token(SyntaxKind kind, SimpleKind token, SourceLocation location) {
    NodeId n=node(kind,location); nodes[n].payload=SyntaxPayload::token; nodes[n].token=token; return n;
}
NodeId SyntaxTree::text(SyntaxKind kind, const std::string& text, SourceLocation location) {
    NodeId n=node(kind,location); nodes[n].payload=SyntaxPayload::spelling;
    nodes[n].offset=spellings.size(); nodes[n].length=text.size();
    spellings.insert(spellings.end(),text.begin(),text.end()); return n;
}
NodeId SyntaxTree::literal(const PostToken& t) {
    NodeId n=text(SyntaxKind::Literal,t.source,t.location);
    nodes[n].range=t.range; nodes[n].payload=SyntaxPayload::literal;
    SyntaxLiteral value={t.kind,t.type,t.suffix,t.scalar,static_cast<std::uint32_t>(values.size()),0,t.width,t.elements};
    if (t.units) {
        value.length=t.width*t.elements;
        values.insert(values.end(),t.units,t.units+value.length);
    }
    nodes[n].literal=literals.size(); literals.push_back(value); return n;
}
void SyntaxTree::append(NodeId parent, NodeId child) {
    if (!child) return;
    const std::uint32_t e=edges.size(); edges.push_back({child,0});
    if (nodes[parent].last) edges[nodes[parent].last].next=e;
    else nodes[parent].first=e;
    nodes[parent].last=e;
}
NodeId SyntaxTree::child(NodeId parent) const { return nodes[parent].first ? edges[nodes[parent].first].child : 0; }
std::string SyntaxTree::compact(NodeId id, const IdentifierTable& ids, bool omit_root_typename, bool omit_root_template) const {
    // Explicit rendering stack keeps deeply nested expressions off the C++ stack.
    struct Item { NodeId id; const char* text; };
    std::vector<Item> work; work.push_back({id,nullptr}); std::string out;
    while (!work.empty()) {
        Item i=work.back(); work.pop_back();
        if (!i.id) { out+=i.text; continue; }
        const auto& n=nodes[i.id];
        auto children=[&]() {
            std::vector<NodeId> list;
            for (auto e=n.first;e;e=edges[e].next) list.push_back(edges[e].child);
            return list;
        };
        if (n.kind==SyntaxKind::SpecialDeclaration || n.kind==SyntaxKind::SpecialDefinition) {
            NodeId d=child(i.id);
            if (nodes[d].kind==SyntaxKind::DeclSpecifiers || nodes[d].kind==SyntaxKind::MemberSpecifiers) {
                auto e=edges[n.first].next; d=e ? edges[e].child : 0;
            }
            for (auto e=nodes[d].first;e;e=edges[e].next)
                if (nodes[edges[e].child].kind==SyntaxKind::Identifier) { work.push_back({edges[e].child,nullptr}); break; }
        } else if (n.operator_literal || n.operator_conversion) {
            out+=n.operator_literal ? "operator\"\"" : "operator";
            work.push_back({child(i.id),nullptr});
            if (n.operator_conversion && n.global_scope) out+=' ';
            if (n.operator_literal && n.last && nodes[edges[n.last].child].kind==SyntaxKind::TemplateArguments) work.insert(work.end()-1,{edges[n.last].child,nullptr});
        } else if (n.destructor) {
            out+='~'; auto spelling=ids.spelling(n.name); out.append(spelling.data,spelling.size);
        } else if (n.is_operator) {
            out+="operator"; out+=token_spelling(n.token);
            if (n.token==SimpleKind::OP_LPAREN) out+=')';
            if (n.token==SimpleKind::OP_LSQUARE) out+=']';
            if (n.operator_array) out+="[]";
            if (n.first) work.push_back({child(i.id),nullptr});
        } else if (n.is_decltype) {
            auto c=children();
            for (std::size_t j=c.size();j>1;--j) { work.push_back({c[j-1],nullptr}); work.push_back({0,"::"}); }
            if (n.typename_keyword) out+="typename ";
            out+="decltype("; work.push_back({0,")"}); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::Trait) {
            out+=token_spelling(n.token); out+='('; work.push_back({0,")"}); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::SizeofPack) {
            out+="sizeof...("; auto spelling=ids.spelling(n.name); out.append(spelling.data,spelling.size); out+=')';
        } else if (n.kind==SyntaxKind::Sizeof) {
            out+="sizeof("; work.push_back({0,")"}); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::TemplateArguments) {
            out+="<"; work.push_back({0,">"}); auto c=children();
            for (std::size_t j=c.size();j>0;--j) { work.push_back({c[j-1],nullptr}); if (j>1) work.push_back({0,","}); }
        } else if ((n.kind==SyntaxKind::PackExpansion || n.kind==SyntaxKind::PackExpression) && n.first) {
            work.push_back({0,"..."}); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::FactoredSyntax || n.kind==SyntaxKind::TypeId || n.kind==SyntaxKind::TypeSpecifiers || n.kind==SyntaxKind::AbstractDeclarator) {
            if (n.typename_keyword && !(omit_root_typename && i.id==id)) out+="typename ";
            auto c=children();
            for (std::size_t j=c.size();j>0;--j) {
                if (n.kind==SyntaxKind::TypeSpecifiers && nodes[c[j-1]].kind==SyntaxKind::CvQualifier) work.push_back({0," "});
                work.push_back({c[j-1],nullptr});
            }
        } else if (n.kind==SyntaxKind::NestedDeclarator) {
            out+='('; work.push_back({0,")"}); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::Parameters) {
            out+='('; work.push_back({0,")"}); auto c=children();
            for (std::size_t j=c.size();j>0;--j) { work.push_back({c[j-1],nullptr}); if (j>1) work.push_back({0,","}); }
        } else if (n.kind==SyntaxKind::Cast && n.token==SimpleKind::OP_LPAREN) {
            auto c=children(); if (c.size()>1) work.push_back({c[1],nullptr});
            work.push_back({0,")"}); work.push_back({c[0],nullptr}); out+='(';
        } else if (n.kind==SyntaxKind::New) {
            auto c=children();
            for (auto j=c.rbegin();j!=c.rend();++j) if (nodes[*j].kind!=SyntaxKind::GlobalScope) work.push_back({*j,nullptr});
            if (!c.empty() && nodes[c[0]].kind==SyntaxKind::GlobalScope) out+="::";
            out+="new";
        } else if (n.kind==SyntaxKind::Placement) {
            work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::GlobalScope) { out+="::";
        } else if (n.kind==SyntaxKind::Initializer) {
            work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::Parameter || n.kind==SyntaxKind::DeclSpecifiers || n.kind==SyntaxKind::Declarator) {
            auto c=children();
            for (std::size_t j=c.size();j>0;--j) { work.push_back({c[j-1],nullptr}); if (n.kind==SyntaxKind::DeclSpecifiers && j>1) work.push_back({0," "}); }
        } else if (n.kind==SyntaxKind::BracedInit) {
            out+='{'; work.push_back({0,"}"}); auto c=children();
            for (std::size_t j=c.size();j>0;--j) { work.push_back({c[j-1],nullptr}); if (j>1) work.push_back({0,","}); }
        } else if (n.kind==SyntaxKind::Parenthesized || n.kind==SyntaxKind::Arguments || n.kind==SyntaxKind::ParenArguments || n.kind==SyntaxKind::ParenInitializer || n.kind==SyntaxKind::LambdaIntroducer) {
            out+=n.kind==SyntaxKind::LambdaIntroducer ? "[" : "(";
            work.push_back({0,n.kind==SyntaxKind::LambdaIntroducer ? "]" : ")"});
            auto c=children();
            for (std::size_t j=c.size();j>0;--j) { work.push_back({c[j-1],nullptr}); if (j>1) work.push_back({0,","}); }
        } else if (n.kind==SyntaxKind::Unary) {
            out+=token_spelling(n.token); work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::Binary || n.kind==SyntaxKind::Assignment || n.kind==SyntaxKind::Comma) {
            auto c=children(); work.push_back({c[1],nullptr}); work.push_back({0,n.kind==SyntaxKind::Comma ? "," : token_spelling(n.token)}); work.push_back({c[0],nullptr});
        } else if (n.kind==SyntaxKind::ArraySuffix) {
            out+='['; work.push_back({0,"]"}); if (n.first) work.push_back({child(i.id),nullptr});
        } else if (n.kind==SyntaxKind::Conditional) {
            auto c=children(); work.push_back({c[2],nullptr}); work.push_back({0,":"}); work.push_back({c[1],nullptr}); work.push_back({0,"?"}); work.push_back({c[0],nullptr});
        } else if (n.kind==SyntaxKind::Subscript) {
            auto c=children(); work.push_back({0,"]"}); work.push_back({c[1],nullptr}); work.push_back({0,"["}); work.push_back({c[0],nullptr});
        } else if (n.kind==SyntaxKind::Member) {
            auto c=children(); work.push_back({c[1],nullptr}); work.push_back({0,token_spelling(n.token)}); work.push_back({c[0],nullptr});
        } else if (n.kind==SyntaxKind::Call) {
            auto c=children(); for (auto j=c.rbegin();j!=c.rend();++j) work.push_back({*j,nullptr});
        } else if (n.kind==SyntaxKind::Capture) {
            if (n.payload==SyntaxPayload::raw_token) out+=token_spelling(n.token);
            auto c=children(); for (auto j=c.rbegin();j!=c.rend();++j) work.push_back({*j,nullptr});
        } else {
            if (n.payload==SyntaxPayload::identifier) {
                if (n.global_scope) out+="::";
                if (n.template_keyword && !(omit_root_template && i.id==id)) out+="template ";
                if (n.typename_keyword && !(omit_root_typename && i.id==id)) out+="typename ";
                auto spelling=ids.spelling(n.name); out.append(spelling.data,spelling.size);
                if (n.kind==SyntaxKind::IdExpression || n.kind==SyntaxKind::Identifier || n.kind==SyntaxKind::TypeName || n.kind==SyntaxKind::QualifiedTypeName || n.kind==SyntaxKind::DeclSpecifier || n.kind==SyntaxKind::Pointer || n.kind==SyntaxKind::Target || n.kind==SyntaxKind::BaseName || n.kind==SyntaxKind::MemInitializerId || n.kind==SyntaxKind::Class || n.kind==SyntaxKind::ClassForward) {
                    if (n.member_pointer) work.push_back({0,"::*"});
                    auto c=children();
                    for (std::size_t j=c.size();j>0;--j) {
                        if (nodes[c[j-1]].kind!=SyntaxKind::Identifier && nodes[c[j-1]].kind!=SyntaxKind::TemplateArguments) continue;
                        work.push_back({c[j-1],nullptr});
                        if (nodes[c[j-1]].kind!=SyntaxKind::TemplateArguments) work.push_back({0,"::"});
                    }
                }
            } else if (n.payload==SyntaxPayload::token || n.payload==SyntaxPayload::raw_token) out+=token_spelling(n.token);
            else if (n.payload==SyntaxPayload::spelling || n.payload==SyntaxPayload::literal) out.append(spellings.data()+n.offset,n.length);
        }
    }
    return out;
}
void SyntaxTree::dump(std::ostream& out, const IdentifierTable& ids, NodeId root) const {
    struct Frame { NodeId node; std::uint32_t edge; unsigned depth; };
    std::vector<Frame> stack;
    // One reusable view buffer and one write per node, not one stream sentry
    // per indentation level. Storage is bounded by maximum rendering depth.
    std::string indentation;
    auto print=[&](NodeId id, unsigned depth) {
        const auto& n=nodes[id];
        const std::size_t width=std::size_t(depth)*2;
        if (indentation.size()<width) indentation.resize(width,' ');
        out.write(indentation.data(),width);
        out << kind_name(n.kind);
        if (n.kind==SyntaxKind::SpecialDeclaration || n.kind==SyntaxKind::SpecialDefinition) out << ' ' << compact(id,ids);
        if (n.is_decltype) out << ' ' << compact(id,ids);
        if (n.destructor || n.is_operator || n.member_pointer || n.operator_literal || n.operator_conversion) out << ' ' << compact(id,ids);
        else if (n.kind==SyntaxKind::TrailingReturn && n.payload==SyntaxPayload::identifier) out << ' ' << compact(child(id),ids);
        else if (n.payload==SyntaxPayload::identifier) {
            if ((n.kind==SyntaxKind::DeclSpecifier || n.kind==SyntaxKind::VirtSpecifier) && !n.first && !n.global_scope) out << " TT_IDENTIFIER:";
            else out << ' ';
            if (n.first && (n.kind==SyntaxKind::IdExpression || n.kind==SyntaxKind::Identifier || n.kind==SyntaxKind::TypeName || n.kind==SyntaxKind::QualifiedTypeName || n.kind==SyntaxKind::DeclSpecifier || n.kind==SyntaxKind::Target || n.kind==SyntaxKind::BaseName || n.kind==SyntaxKind::MemInitializerId || n.kind==SyntaxKind::Class || n.kind==SyntaxKind::ClassForward)) out << compact(id,ids,n.kind==SyntaxKind::IdExpression,n.kind==SyntaxKind::QualifiedTypeName);
            else {
                if (n.global_scope) out << "::";
                auto spelling=ids.spelling(n.name); out.write(spelling.data,spelling.size);
            }
        } else if (n.payload==SyntaxPayload::token) {
            out << ' ' << simple_name(n.token) << ':';
            // PA5's C-style cast leaf intentionally prints the token category only.
            if (!(n.kind==SyntaxKind::Cast && n.token==SimpleKind::OP_LPAREN)) out << token_spelling(n.token);
        } else if (n.payload==SyntaxPayload::raw_token) {
            out << ' ' << token_spelling(n.token);
            if (n.kind==SyntaxKind::FunctionQualifier && n.has_parentheses) {
                out << '(';
                bool first=true;
                for (auto e=n.first;e;e=edges[e].next) { if (!first) out << ','; first=false; out << compact(edges[e].child,ids); }
                out << ')';
            }
        }
        else if (n.payload==SyntaxPayload::spelling || n.payload==SyntaxPayload::literal) {
            out << (n.token_literal_view ? " TT_LITERAL:" : " "); out.write(spellings.data()+n.offset,n.length);
        }
        if (n.kind==SyntaxKind::LambdaIntroducer) out << ' ' << compact(id,ids);
        if (n.kind==SyntaxKind::TrailingReturn && !n.lambda_return && n.first && nodes[child(child(child(id)))].is_decltype) out << ' ' << compact(child(id),ids);
        if (n.kind==SyntaxKind::Placement) out << ' ' << compact(child(id),ids);
        out << '\n';
    };
    auto visible_edges=[&](NodeId n) {
        const auto& node=nodes[n];
        if (node.kind==SyntaxKind::TypeName || node.kind==SyntaxKind::QualifiedTypeName || node.kind==SyntaxKind::IdExpression || node.kind==SyntaxKind::Identifier || node.kind==SyntaxKind::Target || node.kind==SyntaxKind::BaseName || node.kind==SyntaxKind::MemInitializerId || node.kind==SyntaxKind::LambdaIntroducer ||
            node.member_pointer || (node.payload==SyntaxPayload::identifier && node.kind==SyntaxKind::DeclSpecifier) ||
            (node.kind==SyntaxKind::FunctionQualifier && node.token==SimpleKind::KW_THROW)) return std::uint32_t(0);
        return node.first;
    };
    print(root,0); stack.push_back({root,visible_edges(root),0});
    while (!stack.empty()) {
        auto& f=stack.back();
        if (!f.edge) { stack.pop_back(); continue; }
        NodeId c=edges[f.edge].child; f.edge=edges[f.edge].next;
        if (nodes[c].kind==SyntaxKind::TemplateArguments) continue;
        if (nodes[c].kind==SyntaxKind::Attribute) continue; // explicit PA5 view omits attributes
        unsigned depth=f.depth+(nodes[f.node].kind==SyntaxKind::FactoredSyntax ? 0 : 1);
        if (nodes[c].kind!=SyntaxKind::FactoredSyntax) print(c,depth);
        stack.push_back({c,visible_edges(c),depth});
    }
}
}
