#include "syntax/parser.h"
#include <algorithm>
namespace cppgm {
const PostToken& SyntaxParser::peek(unsigned offset) {
    // No cursor advancement or out-of-bounds borrowing after sticky failure.
    if (expected_) { static const PostToken end=[] { PostToken token; token.kind=PostKind::eof; return token; }(); return end; }
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
SyntaxParser::InputToken SyntaxParser::take() { if (expected_) return {}; peek(); InputToken t=std::move(lookahead_.front()); lookahead_.pop_front(); if (!t.name_node && !t.counted) ++tokens_; tree_.anchor=t.location;
    if (t.kind==PostKind::simple) {
        if (t.simple==SimpleKind::OP_LPAREN || t.simple==SimpleKind::OP_LSQUARE || t.simple==SimpleKind::OP_LBRACE) ++delimiter_depth_;
        else if ((t.simple==SimpleKind::OP_RPAREN || t.simple==SimpleKind::OP_RSQUARE || t.simple==SimpleKind::OP_RBRACE) && delimiter_depth_) --delimiter_depth_;
    }
    return t; }
bool SyntaxParser::at(SimpleKind kind, unsigned offset) { if (expected_) return false; const auto& t=peek(offset); return t.kind==PostKind::simple && t.simple==kind; }
bool SyntaxParser::eat(SimpleKind kind) { if (!at(kind)) return false; take(); return true; }
bool SyntaxParser::require(SimpleKind kind) {
    if (expected_) return false;
    if (eat(kind)) return true;
    error(simple_name(kind)); return false;
}
NodeId SyntaxParser::error(const char* message) {
    // Expected grammar rejection is a compact TU-local result. Render only at
    // the explicit driver boundary, never allocate/throw during category work.
    if (!expected_) { error_location_=peek().location; expected_=message; }
    return 0;
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
        Category c=lookahead_[offset].name_node ? tree_.nodes[lookahead_[offset].name_node].category : category(t.identifier);
        if (c==Category::templ && !lookahead_[offset].name_node) { auto binding=lookup(t.identifier,active_.back()); if (binding.category==Category::templ && !binding.template_type) return false; }
        return c==Category::type || c==Category::templ;
    }
    return false;
}
NodeId SyntaxParser::leaf(SyntaxKind k, const PostToken& t) {
    if (expected_) return 0;
    NodeId n=t.kind==PostKind::identifier ? tree_.name(k,t.identifier,t.location) : tree_.token(k,t.simple,t.location);
    tree_.nodes[n].range=t.range; return n;
}
NodeId SyntaxParser::raw(SyntaxKind k, SimpleKind token) {
    if (expected_) return 0;
    NodeId n=tree_.token(k,token); tree_.nodes[n].payload=SyntaxPayload::raw_token; return n;
}
NodeId SyntaxParser::name(SyntaxKind kind) {
    if (expected_) return 0;
    if (peek().kind!=PostKind::identifier) return error("identifier");
    auto token=take();
    if (token.name_node) { tree_.nodes[token.name_node].kind=kind; return token.name_node; }
    return leaf(kind,token);
}
NodeId SyntaxParser::specs(bool type, bool force, NodeId result, bool has_type, bool allow_special) {
    if (expected_) return 0;
    if (!result) result=tree_.node(type ? SyntaxKind::TypeSpecifiers : SyntaxKind::DeclSpecifiers);
    while (!expected_ && (true)) {
        prepare_name();
        const auto t=peek();
        if (t.kind==PostKind::simple && specifier(t.simple)) {
            take(); if (builtin(t.simple)) has_type=true;
            SyntaxKind k=type ? (t.simple==SimpleKind::KW_CONST || t.simple==SimpleKind::KW_VOLATILE ? SyntaxKind::CvQualifier : SyntaxKind::TypeSpecifier) : SyntaxKind::DeclSpecifier;
            tree_.append(result,leaf(k,t));
        } else if ((peek().kind==PostKind::identifier && peek().identifier==attribute_id_) || at(SimpleKind::KW_ALIGNAS) || (at(SimpleKind::OP_LSQUARE) && at(SimpleKind::OP_LSQUARE,1))) {
            attributes(result);
        } else if (!has_type && at(SimpleKind::KW_DECLTYPE)) {
            has_type=true;
            tree_.append(result,decltype_name(type ? SyntaxKind::Decltype : SyntaxKind::DeclSpecifier));
        } else if (!has_type && eat(SimpleKind::KW_TYPENAME)) {
            has_type=true;
            auto kind=type ? SyntaxKind::TypeName : SyntaxKind::DeclSpecifier;
            tree_.append(result,at(SimpleKind::KW_DECLTYPE) ? decltype_name(kind,true) : qualified(kind));
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
    if (!has_type && !(allow_special && special_start())) return error("type specifier");
    tree_.nodes[result].category=has_type ? Category::type : Category::unknown;
    return result;
}
IdentifierId SyntaxParser::declared_name(NodeId n) const {
    // Only declarator-local edges are visited; no TU scan or recursion.
    while (!expected_ && (n)) {
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
NodeId SyntaxParser::parameters() { return parameter_region(); }
NodeId SyntaxParser::parameter_region(bool mixed) {
    if (expected_) return 0;
    if (!require(SimpleKind::OP_LPAREN)) { return 0; }
    NodeId result=tree_.node(SyntaxKind::Parameters);
    parameter_entries(result,mixed);
    if (!require(SimpleKind::OP_RPAREN)) return 0;
    return result;
}
void SyntaxParser::parameter_entries(NodeId result, bool mixed, bool tail) {
    while (!expected_ && (tail ? peek().kind!=PostKind::eof : !at(SimpleKind::OP_RPAREN))) {
        if (eat(SimpleKind::OP_DOTS)) { tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS)); break; }
        if (mixed && !type_start() && !(at(SimpleKind::OP_LSQUARE) && at(SimpleKind::OP_LSQUARE,1)) &&
            !(peek().kind==PostKind::identifier && peek().identifier==attribute_id_)) {
            NodeId item=at(SimpleKind::OP_LBRACE) ? initializer() : expression(2);
            if (tree_.nodes[result].kind==SyntaxKind::Parameters) {
                tree_.nodes[result].kind=SyntaxKind::ParenInitializer;
                for (auto e=tree_.nodes[result].first;e;e=tree_.edges[e].next)
                    tree_.edges[e].child=parameter_expression(tree_.edges[e].child);
            }
            tree_.append(result,item);
            if (!eat(SimpleKind::OP_COMMA)) break;
            continue;
        }
        NodeId p=tree_.node(SyntaxKind::Parameter);
        NodeId attrs=attributes();
        tree_.append(p,specs());
        if (attrs) tree_.append(p,attrs);
        if (!at(SimpleKind::OP_COMMA) && !at(SimpleKind::OP_RPAREN) && !at(SimpleKind::OP_ASS)) {
            const bool anonymous_function=at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_DOTS,1) || at(SimpleKind::OP_RPAREN,1));
            enter(); NodeId d=declarator(anonymous_function); leave();
            tree_.append(p,d);
            IdentifierId id=declared_name(d);
            if (id) {
                bind(id,Category::value);
                tree_.scopes[active_.back()].names[id].parameter_order=++parameter_order_;
            }
            tree_.nodes[p].scope=active_.back();
        }
        if (eat(SimpleKind::OP_ASS)) {
            NodeId a=tree_.node(SyntaxKind::DefaultArgument);
            tree_.append(p,a);
            if (!classes_.empty() && !tail) {
                tree_.append(result,p);
                // Capture the rest of this clause as one deferred grammar region.
                // Commas inside a later template-id need no angle guess here.
                defer_expression(a,DeferredKind::parameter_tail,SimpleKind::OP_RPAREN,result);
                return;
            }
            tree_.append(a,initializer(true));
        }
        tree_.append(result,tree_.nodes[result].kind==SyntaxKind::Parameters ? p : parameter_expression(p));
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
}

NodeId SyntaxParser::declarator(bool abstract, bool allow_name, bool allocation) {
    if (expected_) return 0;
    NodeId result=tree_.node(abstract ? SyntaxKind::AbstractDeclarator : SyntaxKind::Declarator);
    while (!expected_ && (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) ||
        (peek().kind==PostKind::identifier && (at(SimpleKind::OP_COLON2,1) || (lookahead_.front().name_node && tree_.nodes[lookahead_.front().name_node].member_pointer))))) {
        if (peek().kind==PostKind::identifier) {
            NodeId p=qualified(SyntaxKind::Pointer);
            if (!tree_.nodes[p].member_pointer) return error("member pointer operator");
            tree_.append(result,p);
        } else tree_.append(result,leaf(SyntaxKind::Pointer,take()));
        while (!expected_ && (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE))) tree_.append(result,leaf(SyntaxKind::CvQualifier,take()));
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    if (allow_name && (peek().kind==PostKind::identifier || at(SimpleKind::KW_OPERATOR) || at(SimpleKind::OP_COMPL))) {
        NodeId n=qualified(SyntaxKind::Identifier); tree_.append(result,n);
        if (tree_.nodes[n].qualifier_scope) qualify_scope(tree_.nodes[n].qualifier_scope);
    }
    else if (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || (peek(1).kind==PostKind::identifier && (!type_start(1) || at(SimpleKind::OP_COLON2,2))))) {
        take(); NodeId nested=tree_.node(SyntaxKind::NestedDeclarator);
        tree_.append(nested,declarator(abstract,allow_name)); if (!require(SimpleKind::OP_RPAREN)) { return 0; } tree_.append(result,nested);
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    return suffixes(result,allocation);
}
NodeId SyntaxParser::suffixes(NodeId result, bool allocation) {
    if (expected_) return 0;
    while (!expected_ && (true)) {
        attributes(result);
        if (eat(SimpleKind::OP_LSQUARE)) {
            NodeId a=tree_.node(SyntaxKind::ArraySuffix);
            if (!at(SimpleKind::OP_RSQUARE)) tree_.append(a,expression(2));
            if (!require(SimpleKind::OP_RSQUARE)) { return 0; } tree_.append(result,a);
        } else if (!allocation && at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_RPAREN,1) || at(SimpleKind::OP_DOTS,1) ||
            (at(SimpleKind::OP_LSQUARE,1) && at(SimpleKind::OP_LSQUARE,2)) ||
            (peek(1).kind==PostKind::identifier && peek(1).identifier==attribute_id_) || type_start(1))) {
            NodeId region=parameter_region(tree_.nodes[result].kind==SyntaxKind::Declarator);
            if (tree_.nodes[region].kind==SyntaxKind::ParenInitializer) {
                NodeId init=tree_.node(SyntaxKind::Initializer); tree_.append(init,region); tree_.nodes[result].initializer=init; break;
            }
            tree_.append(result,region);
            while (!expected_ && (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::KW_NOEXCEPT) || at(SimpleKind::KW_THROW))) {
                PostToken t=take(); NodeId q=(t.simple==SimpleKind::KW_CONST || t.simple==SimpleKind::KW_VOLATILE) ? leaf(SyntaxKind::CvQualifier,t) : raw(SyntaxKind::FunctionQualifier,t.simple);
                if ((t.simple==SimpleKind::KW_NOEXCEPT || t.simple==SimpleKind::KW_THROW) && eat(SimpleKind::OP_LPAREN)) {
                    if (t.simple==SimpleKind::KW_NOEXCEPT) {
                        if (!classes_.empty()) defer_expression(q,DeferredKind::expression,SimpleKind::OP_RPAREN);
                        else tree_.append(q,expression());
                    }
                    else {
                        while (!expected_ && (!at(SimpleKind::OP_RPAREN))) {
                            tree_.append(q,type_id()); if (!eat(SimpleKind::OP_COMMA)) break;
                        }
                    }
                    if (!require(SimpleKind::OP_RPAREN)) { return 0; }
                    tree_.nodes[q].has_parentheses=true; // qualifier has a parenthesized operand, including empty throw()
                }
                tree_.append(result,q);
            }
            while (!expected_ && (peek().kind==PostKind::identifier && (peek().identifier==override_id_ || peek().identifier==final_id_))) tree_.append(result,name(SyntaxKind::VirtSpecifier));
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
    if (expected_) return 0;
    bool typename_keyword=at(SimpleKind::KW_TYPENAME);
    NodeId result=tree_.node(SyntaxKind::TypeId); tree_.nodes[result].typename_keyword=typename_keyword; tree_.append(result,specs(true,force));
    prepare_name();
    if ((peek().kind==PostKind::identifier && lookahead_.front().name_node && tree_.nodes[lookahead_.front().name_node].member_pointer) ||
        at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::OP_LSQUARE) ||
        (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || (!allocation && type_start(1)) || (!allocation && at(SimpleKind::OP_RPAREN,1))))) tree_.append(result,declarator(true,false,allocation));
    return result;
}
NodeId SyntaxParser::list(SyntaxKind kind, SimpleKind close) {
    if (expected_) return 0;
    NodeId result=tree_.node(kind);
    while (!expected_ && (!at(close))) {
        NodeId item=at(SimpleKind::OP_LBRACE) ? initializer() : expression(2);
        if (eat(SimpleKind::OP_DOTS)) { NodeId pack=tree_.node(SyntaxKind::PackExpression); tree_.append(pack,item); item=pack; }
        tree_.append(result,item);
        if (!eat(SimpleKind::OP_COMMA)) break;
        if (close==SimpleKind::OP_RBRACE && at(close)) break;
    }
    if (!require(close)) { return 0; } return result;
}
NodeId SyntaxParser::initializer(bool equal) {
    if (expected_) return 0;
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
    if (expected_) return 0;
    NodeId attrs=attributes(); NodeId result=declaration_impl();
    if (attrs) tree_.append(result,attrs);
    return result;
}
NodeId SyntaxParser::declaration_impl() {
    if (at(SimpleKind::KW_TEMPLATE) || (at(SimpleKind::KW_EXTERN) && at(SimpleKind::KW_TEMPLATE,1))) return template_declaration();
    if (expected_) return 0;
    if (at(SimpleKind::KW_NAMESPACE) || (at(SimpleKind::KW_INLINE) && at(SimpleKind::KW_NAMESPACE,1))) return namespace_declaration();
    if (at(SimpleKind::KW_ENUM)) {
        NodeId e=enum_specifier();
        if (eat(SimpleKind::OP_SEMICOLON)) {
            if (classes_.empty()) return e;
            NodeId result=tree_.node(SyntaxKind::SimpleDeclaration);
            NodeId spec=tree_.node(SyntaxKind::DeclSpecifiers);
            tree_.append(spec,e); tree_.append(result,spec); return result;
        }
        // The common declaration path also handles enum object declarators.
        NodeId s=tree_.node(SyntaxKind::DeclSpecifiers); tree_.append(s,e);
        return declaration_tail(specs(false,false,s,true),false);
    }
    if (eat(SimpleKind::OP_SEMICOLON)) return tree_.node(SyntaxKind::EmptyDeclaration);
    if (eat(SimpleKind::KW_STATIC_ASSERT)) {
        NodeId result=tree_.node(SyntaxKind::StaticAssert); if (!require(SimpleKind::OP_LPAREN)) { return 0; }
        tree_.append(result,expression(2)); if (!require(SimpleKind::OP_COMMA)) { return 0; }
        auto t=take(); if (t.kind!=PostKind::array) return error("string literal");
        NodeId message=t.literal_node; tree_.nodes[message].kind=SyntaxKind::Message; tree_.append(result,message);
        if (!require(SimpleKind::OP_RPAREN)) { return 0; } if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
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
            while (!expected_ && (!at(SimpleKind::OP_RBRACE))) tree_.append(result,declaration());
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
    if (expected_) return 0;
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
    while (!expected_ && (true)) {
        enter(); // parameters' scope extends through this function's body, not sibling declarations
        const bool unnamed_bit=at(SimpleKind::OP_COLON);
        NodeId d=unnamed_bit ? 0 : declarator(); if (d) tree_.nodes[d].scope=active_.back(); IdentifierId id=declared_name(d);
        if (!id && !at(SimpleKind::OP_COLON)) {
            bool operator_name=false;
            for (auto e=tree_.nodes[d].first;e;e=tree_.edges[e].next) operator_name |= tree_.nodes[tree_.edges[e].child].is_operator || tree_.nodes[tree_.edges[e].child].operator_literal || tree_.nodes[tree_.edges[e].child].operator_conversion;
            if (!operator_name) return error("named declarator");
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
            if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
        }
        leave();
        if (!instantiation_depth_) bind(id,typedef_decl ? Category::type : Category::value,typedef_decl ? alias_target : 0);
        NodeId init=tree_.node(SyntaxKind::InitDeclarator); tree_.append(init,d);
        if (tree_.nodes[d].initializer) tree_.append(init,tree_.nodes[d].initializer);
        else if (eat(SimpleKind::OP_ASS)) {
            if (!classes_.empty() && active_.back()==classes_.back().scope && !function_declarator(d)) {
                if (!init_list) init_list=tree_.node(SyntaxKind::InitDeclarators);
                defer_expression(init,DeferredKind::member_initializer_tail,SimpleKind::OP_SEMICOLON,init_list);
            }
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
    if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } tree_.append(result,init_list); return result;
}
NodeId SyntaxParser::parse() {
    if (expected_) return 0;
    tree_.anchor=peek().location;
    NodeId root=tree_.node(SyntaxKind::TranslationUnit); tree_.nodes[root].scope=active_.back();
    while (!expected_ && (peek().kind!=PostKind::eof)) tree_.append(root,declaration());
    return expected_ ? 0 : root;
}
}
