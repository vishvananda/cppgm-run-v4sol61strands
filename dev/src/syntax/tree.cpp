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
    SyntaxNode n; n.kind=kind; n.location=location;
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
void SyntaxTree::dump(std::ostream& out, const IdentifierTable& ids, NodeId root) const {
    struct Frame { NodeId node; std::uint32_t edge; unsigned depth; };
    std::vector<Frame> stack;
    auto print=[&](NodeId id, unsigned depth) {
        const auto& n=nodes[id];
        for (unsigned i=0;i<depth;++i) out << "  ";
        out << kind_name(n.kind);
        if (n.payload==SyntaxPayload::identifier) {
            auto spelling=ids.spelling(n.name); out << ' '; out.write(spelling.data,spelling.size);
        } else if (n.payload==SyntaxPayload::token) {
            out << ' ' << simple_name(n.token) << ':';
            // PA5's C-style cast leaf intentionally prints the token category only.
            if (!(n.kind==SyntaxKind::Cast && n.token==SimpleKind::OP_LPAREN)) out << token_spelling(n.token);
        } else if (n.payload==SyntaxPayload::raw_token) out << ' ' << token_spelling(n.token);
        else if (n.payload==SyntaxPayload::spelling || n.payload==SyntaxPayload::literal) {
            out << ' '; out.write(spellings.data()+n.offset,n.length);
        }
        out << '\n';
    };
    print(root,0); stack.push_back({root,nodes[root].first,0});
    while (!stack.empty()) {
        auto& f=stack.back();
        if (!f.edge) { stack.pop_back(); continue; }
        NodeId c=edges[f.edge].child; f.edge=edges[f.edge].next;
        unsigned depth=f.depth+1; print(c,depth); stack.push_back({c,nodes[c].first,depth});
    }
}
}
