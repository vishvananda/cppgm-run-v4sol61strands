#include "syntax/parser.h"
#include <algorithm>
#include <stdexcept>
namespace cppgm {
const PostToken& SyntaxParser::peek(unsigned offset) {
    while (lookahead_.size()<=offset) {
        InputToken t;
        if (deferred_input_) {
            if (deferred_position_==deferred_input_->size()) t.kind=PostKind::eof;
            else {
                const auto& d=(*deferred_input_)[deferred_position_++];
                t.kind=d.kind; t.simple=d.simple; t.identifier=d.identifier;
                t.location=d.location; t.range=d.range; t.literal_node=d.literal;
            }
            t.counted=true;
        } else static_cast<PostToken&>(t)=cursor_.next();
        // Literal bytes are captured before advancing the borrowing cursor.
        if (!t.literal_node && t.kind!=PostKind::simple && t.kind!=PostKind::identifier && t.kind!=PostKind::eof && t.kind!=PostKind::invalid) {
            NodeId n=tree_.literal(t); t.literal_node=n; t.units=nullptr; t.numeric_data=nullptr;
        }
        lookahead_.push_back(std::move(t));
        max_lookahead_=std::max(max_lookahead_,lookahead_.size());
    }
    return lookahead_[offset];
}
SyntaxParser::InputToken SyntaxParser::take() { peek(); InputToken t=std::move(lookahead_.front()); lookahead_.pop_front(); if (!t.name_node && !t.counted) ++tokens_; tree_.anchor=t.location; return t; }
bool SyntaxParser::at(SimpleKind kind, unsigned offset) { const auto& t=peek(offset); return t.kind==PostKind::simple && t.simple==kind; }
bool SyntaxParser::eat(SimpleKind kind) { if (!at(kind)) return false; take(); return true; }
void SyntaxParser::require(SimpleKind kind) { if (!eat(kind)) error(simple_name(kind)); }
void SyntaxParser::error(const char* message) {
    const auto& t=peek();
    throw std::runtime_error(std::string("syntax: expected ")+message+" at line "+std::to_string(t.location.line));
}
bool SyntaxParser::builtin(SimpleKind k) const {
    switch (k) {
    case SimpleKind::KW_AUTO: case SimpleKind::KW_BOOL: case SimpleKind::KW_CHAR:
    case SimpleKind::KW_CHAR16_T: case SimpleKind::KW_CHAR32_T: case SimpleKind::KW_DOUBLE:
    case SimpleKind::KW_FLOAT: case SimpleKind::KW_INT: case SimpleKind::KW_LONG:
    case SimpleKind::KW_SHORT: case SimpleKind::KW_SIGNED: case SimpleKind::KW_UNSIGNED:
    case SimpleKind::KW_VOID: case SimpleKind::KW_WCHAR_T: return true;
    default: return false;
    }
}
bool SyntaxParser::specifier(SimpleKind k) const {
    if (builtin(k)) return true;
    switch (k) {
    case SimpleKind::KW_CONST: case SimpleKind::KW_VOLATILE: case SimpleKind::KW_CONSTEXPR:
    case SimpleKind::KW_TYPEDEF: case SimpleKind::KW_STATIC: case SimpleKind::KW_EXTERN:
    case SimpleKind::KW_INLINE: case SimpleKind::KW_REGISTER: case SimpleKind::KW_THREAD_LOCAL:
    case SimpleKind::KW_MUTABLE: case SimpleKind::KW_FRIEND: case SimpleKind::KW_VIRTUAL:
    case SimpleKind::KW_EXPLICIT: return true;
    default: return false;
    }
}
bool SyntaxParser::type_start(unsigned offset) {
    prepare_name(offset);
    const auto& t=peek(offset);
    if (t.kind==PostKind::simple) return specifier(t.simple) || t.simple==SimpleKind::KW_TYPENAME || t.simple==SimpleKind::KW_DECLTYPE || t.simple==SimpleKind::KW_CLASS || t.simple==SimpleKind::KW_STRUCT || t.simple==SimpleKind::KW_UNION || t.simple==SimpleKind::KW_ENUM;
    if (t.kind==PostKind::identifier) {
        if (lookahead_[offset].name_node && tree_.nodes[lookahead_[offset].name_node].member_pointer) return false;
        Category c=lookahead_[offset].name_node ? tree_.nodes[lookahead_[offset].name_node].category : category(t.identifier); return c==Category::type || c==Category::templ;
    }
    return false;
}
NodeId SyntaxParser::leaf(SyntaxKind k, const PostToken& t) {
    NodeId n=t.kind==PostKind::identifier ? tree_.name(k,t.identifier,t.location) : tree_.token(k,t.simple,t.location);
    tree_.nodes[n].range=t.range; return n;
}
NodeId SyntaxParser::raw(SyntaxKind k, SimpleKind token) {
    NodeId n=tree_.token(k,token); tree_.nodes[n].payload=SyntaxPayload::raw_token; return n;
}
NodeId SyntaxParser::name(SyntaxKind kind) {
    if (peek().kind!=PostKind::identifier) error("identifier");
    auto token=take();
    if (token.name_node) { tree_.nodes[token.name_node].kind=kind; return token.name_node; }
    return leaf(kind,token);
}
NodeId SyntaxParser::specs(bool type, bool force, NodeId result, bool has_type, bool allow_special) {
    if (!result) result=tree_.node(type ? SyntaxKind::TypeSpecifiers : SyntaxKind::DeclSpecifiers);
    while (true) {
        prepare_name();
        const auto t=peek();
        if (t.kind==PostKind::simple && specifier(t.simple)) {
            take(); if (builtin(t.simple)) has_type=true;
            SyntaxKind k=type ? (t.simple==SimpleKind::KW_CONST || t.simple==SimpleKind::KW_VOLATILE ? SyntaxKind::CvQualifier : SyntaxKind::TypeSpecifier) : SyntaxKind::DeclSpecifier;
            tree_.append(result,leaf(k,t));
        } else if (!has_type && eat(SimpleKind::KW_DECLTYPE)) {
            has_type=true; require(SimpleKind::OP_LPAREN);
            NodeId n=tree_.node(type ? SyntaxKind::Decltype : SyntaxKind::DeclSpecifier,t.location);
            tree_.nodes[n].is_decltype=true;
            tree_.append(n,expression()); require(SimpleKind::OP_RPAREN); tree_.append(result,n);
        } else if (!has_type && eat(SimpleKind::KW_TYPENAME)) {
            has_type=true; tree_.append(result,qualified(type ? SyntaxKind::TypeName : SyntaxKind::DeclSpecifier));
        } else if (!has_type && at(SimpleKind::KW_ENUM)) {
            has_type=true; tree_.append(result,enum_specifier());
        } else if (!has_type && (at(SimpleKind::KW_CLASS) || at(SimpleKind::KW_STRUCT) || at(SimpleKind::KW_UNION))) {
            has_type=true; tree_.append(result,class_specifier());
        } else if (!has_type && t.kind==PostKind::identifier && !(allow_special && special_start()) && (force || type_start())) {
            has_type=true;
            NodeId n=qualified(type ? SyntaxKind::TypeName : SyntaxKind::DeclSpecifier);
            tree_.append(result,n);
        } else break;
    }
    if (!has_type && !(allow_special && special_start())) error("type specifier");
    tree_.nodes[result].category=has_type ? Category::type : Category::unknown;
    return result;
}
IdentifierId SyntaxParser::declared_name(NodeId n) const {
    // Only declarator-local edges are visited; no TU scan or recursion.
    while (n) {
        NodeId nested=0;
        for (auto e=tree_.nodes[n].first;e;e=tree_.edges[e].next) {
            NodeId c=tree_.edges[e].child;
            if (tree_.nodes[c].kind==SyntaxKind::Identifier) return tree_.nodes[c].terminal_name ? tree_.nodes[c].terminal_name : tree_.nodes[c].name;
            if (tree_.nodes[c].kind==SyntaxKind::NestedDeclarator) nested=tree_.child(c);
        }
        n=nested;
    }
    return 0;
}
bool SyntaxParser::function_declarator(NodeId n) const {
    for (auto e=tree_.nodes[n].first;e;e=tree_.edges[e].next)
        if (tree_.nodes[tree_.edges[e].child].kind==SyntaxKind::Parameters) return true;
    return false;
}
NodeId SyntaxParser::parameters() {
    require(SimpleKind::OP_LPAREN);
    NodeId result=tree_.node(SyntaxKind::Parameters);
    while (!at(SimpleKind::OP_RPAREN)) {
        if (eat(SimpleKind::OP_DOTS)) { tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS)); break; }
        NodeId p=tree_.node(SyntaxKind::Parameter);
        tree_.append(p,specs());
        if (!at(SimpleKind::OP_COMMA) && !at(SimpleKind::OP_RPAREN) && !at(SimpleKind::OP_ASS)) {
            const bool anonymous_function=at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_DOTS,1) || at(SimpleKind::OP_RPAREN,1));
            enter(); NodeId d=declarator(anonymous_function); leave();
            tree_.append(p,d); bind(declared_name(d),Category::value);
        }
        if (eat(SimpleKind::OP_ASS)) {
            NodeId a=tree_.node(SyntaxKind::DefaultArgument);
            if (!classes_.empty()) defer_expression(a,DeferredKind::equal_initializer,SimpleKind::OP_RPAREN);
            else tree_.append(a,initializer(true));
            tree_.append(p,a);
        }
        tree_.append(result,p);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_RPAREN); return result;
}
NodeId SyntaxParser::declarator(bool abstract, bool allow_name, bool allocation) {
    NodeId result=tree_.node(abstract ? SyntaxKind::AbstractDeclarator : SyntaxKind::Declarator);
    while (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) ||
        (peek().kind==PostKind::identifier && (at(SimpleKind::OP_COLON2,1) || (lookahead_.front().name_node && tree_.nodes[lookahead_.front().name_node].member_pointer)))) {
        if (peek().kind==PostKind::identifier) {
            NodeId p=qualified(SyntaxKind::Pointer);
            if (!tree_.nodes[p].member_pointer) error("member pointer operator");
            tree_.append(result,p);
        } else tree_.append(result,leaf(SyntaxKind::Pointer,take()));
        while (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE)) tree_.append(result,leaf(SyntaxKind::CvQualifier,take()));
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    if (allow_name && (peek().kind==PostKind::identifier || at(SimpleKind::KW_OPERATOR))) {
        NodeId n=qualified(SyntaxKind::Identifier); tree_.append(result,n);
        if (tree_.nodes[n].qualifier_scope) tree_.scopes[active_.back()].parent=tree_.nodes[n].qualifier_scope;
    }
    else if (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || (peek(1).kind==PostKind::identifier && (!type_start(1) || at(SimpleKind::OP_COLON2,2))))) {
        take(); NodeId nested=tree_.node(SyntaxKind::NestedDeclarator);
        tree_.append(nested,declarator(abstract,allow_name)); require(SimpleKind::OP_RPAREN); tree_.append(result,nested);
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    return suffixes(result,allocation);
}
NodeId SyntaxParser::suffixes(NodeId result, bool allocation) {
    while (true) {
        attributes();
        if (eat(SimpleKind::OP_LSQUARE)) {
            NodeId a=tree_.node(SyntaxKind::ArraySuffix);
            if (!at(SimpleKind::OP_RSQUARE)) tree_.append(a,expression(2));
            require(SimpleKind::OP_RSQUARE); tree_.append(result,a);
        } else if (!allocation && at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_RPAREN,1) || at(SimpleKind::OP_DOTS,1) || type_start(1))) {
            tree_.append(result,parameters());
            while (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::KW_NOEXCEPT) || at(SimpleKind::KW_THROW)) {
                PostToken t=take(); NodeId q=(t.simple==SimpleKind::KW_CONST || t.simple==SimpleKind::KW_VOLATILE) ? leaf(SyntaxKind::CvQualifier,t) : raw(SyntaxKind::FunctionQualifier,t.simple);
                if ((t.simple==SimpleKind::KW_NOEXCEPT || t.simple==SimpleKind::KW_THROW) && eat(SimpleKind::OP_LPAREN)) {
                    if (t.simple==SimpleKind::KW_NOEXCEPT) {
                        if (!classes_.empty()) defer_expression(q,DeferredKind::expression,SimpleKind::OP_RPAREN);
                        else tree_.append(q,expression());
                    }
                    else {
                        while (!at(SimpleKind::OP_RPAREN)) {
                            tree_.append(q,type_id()); if (!eat(SimpleKind::OP_COMMA)) break;
                        }
                    }
                    require(SimpleKind::OP_RPAREN);
                    tree_.nodes[q].has_parentheses=true; // qualifier has a parenthesized operand, including empty throw()
                }
                tree_.append(result,q);
            }
            while (peek().kind==PostKind::identifier && (peek().identifier==override_id_ || peek().identifier==final_id_)) tree_.append(result,name(SyntaxKind::VirtSpecifier));
            if (eat(SimpleKind::OP_ARROW)) {
                NodeId t=type_id(); NodeId r=tree_.node(SyntaxKind::TrailingReturn);
                // Surface type text remains a rendering view; structure drives later use.
                NodeId s=tree_.child(t), first=tree_.child(s);
                const auto& f=tree_.nodes[first];
                if (f.payload==SyntaxPayload::identifier) { tree_.nodes[r].payload=SyntaxPayload::identifier; tree_.nodes[r].name=f.name; }
                tree_.append(r,t); tree_.append(result,r);
            }
        } else break;
    }
    return result;
}
NodeId SyntaxParser::type_id(bool allocation, bool force) {
    NodeId result=tree_.node(SyntaxKind::TypeId); tree_.append(result,specs(true,force));
    if (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::OP_LSQUARE) ||
        (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || type_start(1) || (!allocation && at(SimpleKind::OP_RPAREN,1))))) tree_.append(result,declarator(true,false,allocation));
    return result;
}
NodeId SyntaxParser::list(SyntaxKind kind, SimpleKind close) {
    NodeId result=tree_.node(kind);
    while (!at(close)) {
        tree_.append(result,at(SimpleKind::OP_LBRACE) ? initializer() : expression(2));
        if (!eat(SimpleKind::OP_COMMA)) break;
        if (close==SimpleKind::OP_RBRACE && at(close)) break;
    }
    require(close); return result;
}
NodeId SyntaxParser::initializer(bool equal) {
    if (eat(SimpleKind::OP_LBRACE)) {
        NodeId b=list(SyntaxKind::BracedInit,SimpleKind::OP_RBRACE);
        if (!equal) return b;
        NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,b); return i;
    }
    NodeId result=tree_.node(SyntaxKind::Initializer);
    if (!equal && eat(SimpleKind::OP_LPAREN)) tree_.append(result,list(SyntaxKind::ParenInitializer,SimpleKind::OP_RPAREN));
    else tree_.append(result,expression(2));
    return result;
}
NodeId SyntaxParser::declaration() {
    if (at(SimpleKind::KW_NAMESPACE) || (at(SimpleKind::KW_INLINE) && at(SimpleKind::KW_NAMESPACE,1))) return namespace_declaration();
    if (at(SimpleKind::KW_ENUM)) {
        NodeId e=enum_specifier();
        if (eat(SimpleKind::OP_SEMICOLON)) return e;
        // The common declaration path also handles enum object declarators.
        NodeId s=tree_.node(SyntaxKind::DeclSpecifiers); tree_.append(s,e);
        return declaration_tail(specs(false,false,s,true),false);
    }
    attributes();
    if (eat(SimpleKind::OP_SEMICOLON)) return tree_.node(SyntaxKind::EmptyDeclaration);
    if (eat(SimpleKind::KW_STATIC_ASSERT)) {
        NodeId result=tree_.node(SyntaxKind::StaticAssert); require(SimpleKind::OP_LPAREN);
        tree_.append(result,expression(2)); require(SimpleKind::OP_COMMA);
        auto t=take(); if (t.kind!=PostKind::array) error("string literal");
        NodeId message=t.literal_node; tree_.nodes[message].kind=SyntaxKind::Message; tree_.append(result,message);
        require(SimpleKind::OP_RPAREN); require(SimpleKind::OP_SEMICOLON); return result;
    }
    if (at(SimpleKind::KW_USING)) return using_declaration();
    if (at(SimpleKind::KW_EXTERN) && peek(1).kind==PostKind::array) {
        take(); auto language=take();
        std::string text=language.source;
        if (text.size()>=2 && text.front()=='"' && text.back()=='"') text=text.substr(1,text.size()-2);
        NodeId result=language.literal_node;
        auto& node=tree_.nodes[result]; node.kind=SyntaxKind::Linkage; node.payload=SyntaxPayload::spelling;
        node.offset=tree_.spellings.size(); node.length=text.size();
        tree_.spellings.insert(tree_.spellings.end(),text.begin(),text.end());
        if (eat(SimpleKind::OP_LBRACE)) {
            while (!at(SimpleKind::OP_RBRACE)) tree_.append(result,declaration());
            take();
        } else tree_.append(result,declaration());
        return result;
    }
    if (at(SimpleKind::KW_CLASS) || at(SimpleKind::KW_STRUCT) || at(SimpleKind::KW_UNION)) {
        NodeId c=class_specifier();
        if (eat(SimpleKind::OP_SEMICOLON)) return c;
        NodeId spec=tree_.node(SyntaxKind::DeclSpecifiers); tree_.append(spec,c);
        return declaration_tail(spec,false);
    }
    prepare_name();
    if (special_start()) return special_member();
    const bool typedef_decl=at(SimpleKind::KW_TYPEDEF);
    return declaration_tail(specs(false,false,0,false,true),typedef_decl);
}
NodeId SyntaxParser::declaration_tail(NodeId spec, bool typedef_decl) {
    if (tree_.nodes[spec].category==Category::unknown && special_start()) return special_member(spec);
    NodeId result=tree_.node(SyntaxKind::SimpleDeclaration); tree_.append(result,spec);
    SyntaxScopeId alias_target=0;
    if (typedef_decl) {
        // Only this declaration's bounded specifier sequence is inspected.
        for (auto e=tree_.nodes[spec].first;e;e=tree_.edges[e].next) {
            const auto& n=tree_.nodes[tree_.edges[e].child];
            if (n.resolved_scope || n.scope) alias_target=n.resolved_scope ? n.resolved_scope : n.scope;
        }
    }
    if (eat(SimpleKind::OP_SEMICOLON)) return result;
    NodeId init_list=0;
    while (true) {
        enter(); // parameters' scope extends through this function's body, not sibling declarations
        const bool unnamed_bit=at(SimpleKind::OP_COLON);
        NodeId d=unnamed_bit ? 0 : declarator(); if (d) tree_.nodes[d].scope=active_.back(); IdentifierId id=declared_name(d);
        if (!id && !at(SimpleKind::OP_COLON)) {
            bool operator_name=false;
            for (auto e=tree_.nodes[d].first;e;e=tree_.edges[e].next) operator_name |= tree_.nodes[tree_.edges[e].child].is_operator || tree_.nodes[tree_.edges[e].child].operator_literal || tree_.nodes[tree_.edges[e].child].operator_conversion;
            if (!operator_name) error("named declarator");
        }
        if (function_declarator(d) && (at(SimpleKind::OP_LBRACE) || at(SimpleKind::KW_TRY))) {
            tree_.nodes[result].kind=SyntaxKind::FunctionDefinition;
            tree_.append(result,d);
            bind(id,Category::value);
            function_body(result);
            leave(); bind(id,Category::value); return result;
        }
        if (eat(SimpleKind::OP_COLON)) {
            tree_.nodes[result].kind=SyntaxKind::BitField;
            NodeId bit=tree_.node(SyntaxKind::BitDeclarator); if (d) tree_.append(bit,d);
            tree_.append(bit,expression(2)); tree_.append(result,bit);
            leave(); bind(id,Category::value);
            if (eat(SimpleKind::OP_COMMA)) continue;
            require(SimpleKind::OP_SEMICOLON); return result;
        }
        leave(); bind(id,typedef_decl ? Category::type : Category::value,typedef_decl ? alias_target : 0);
        NodeId init=tree_.node(SyntaxKind::InitDeclarator); tree_.append(init,d);
        if (eat(SimpleKind::OP_ASS)) {
            if (!classes_.empty() && active_.back()==classes_.back().scope && !function_declarator(d)) defer_expression(init,DeferredKind::equal_initializer,SimpleKind::OP_SEMICOLON);
            else if (at(SimpleKind::KW_DEFAULT) || at(SimpleKind::KW_DELETE)) {
                auto t=take(); NodeId i=tree_.node(SyntaxKind::Initializer);
                tree_.append(i,raw(SyntaxKind::SpecialInitializer,t.simple)); tree_.append(init,i);
            } else tree_.append(init,initializer(true));
        }
        else if (at(SimpleKind::OP_LPAREN)) tree_.append(init,initializer());
        else if (at(SimpleKind::OP_LBRACE)) {
            NodeId i=tree_.node(SyntaxKind::Initializer);
            if (!classes_.empty() && active_.back()==classes_.back().scope) defer_expression(i,DeferredKind::direct_initializer,SimpleKind::OP_SEMICOLON);
            else tree_.append(i,initializer());
            tree_.append(init,i);
        }
        if (!init_list) init_list=tree_.node(SyntaxKind::InitDeclarators);
        tree_.append(init_list,init);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_SEMICOLON); tree_.append(result,init_list); return result;
}
NodeId SyntaxParser::parse() {
    tree_.anchor=peek().location;
    NodeId root=tree_.node(SyntaxKind::TranslationUnit); tree_.nodes[root].scope=active_.back();
    while (peek().kind!=PostKind::eof) tree_.append(root,declaration());
    return root;
}
}
