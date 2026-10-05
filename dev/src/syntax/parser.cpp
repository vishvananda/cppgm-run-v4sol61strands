#include "syntax/parser.h"
#include <algorithm>
#include <stdexcept>
namespace cppgm {
const PostToken& SyntaxParser::peek(unsigned offset) {
    while (lookahead_.size()<=offset) {
        PostToken t=cursor_.next();
        // Literal bytes are captured before advancing the borrowing cursor.
        if (t.kind!=PostKind::simple && t.kind!=PostKind::identifier && t.kind!=PostKind::eof && t.kind!=PostKind::invalid) {
            NodeId n=tree_.literal(t); t.numeric_prefix=n; t.units=nullptr;
        }
        lookahead_.push_back(std::move(t));
        max_lookahead_=std::max(max_lookahead_,lookahead_.size());
    }
    return lookahead_[offset];
}
PostToken SyntaxParser::take() { PostToken t=peek(); lookahead_.pop_front(); ++tokens_; return t; }
bool SyntaxParser::at(SimpleKind kind, unsigned offset) { const auto& t=peek(offset); return t.kind==PostKind::simple && t.simple==kind; }
bool SyntaxParser::eat(SimpleKind kind) { if (!at(kind)) return false; take(); return true; }
void SyntaxParser::require(SimpleKind kind) { if (!eat(kind)) error(simple_name(kind)); }
void SyntaxParser::error(const char* message) {
    const auto& t=peek();
    throw std::runtime_error(std::string("syntax: expected ")+message+" at line "+std::to_string(t.location.line));
}
void SyntaxParser::enter() { scopes_.push_back(changes_.size()); }
void SyntaxParser::leave() {
    while (changes_.size()>scopes_.back()) {
        auto c=changes_.back(); changes_.pop_back(); bindings_[c.id]=c.previous;
    }
    scopes_.pop_back();
}
void SyntaxParser::bind(IdentifierId id, Category value) {
    if (!id) return;
    if (bindings_.size()<=id) bindings_.resize(id+1,Category::unknown);
    changes_.push_back({id,bindings_[id]}); bindings_[id]=value;
}
SyntaxParser::Category SyntaxParser::category(IdentifierId id) {
    ++queries_;
    if (id<bindings_.size() && bindings_[id]!=Category::unknown) return bindings_[id];
    // Lexical fallback is computed once per interned identifier, not per query.
    if (hints_.size()<=id) hints_.resize(id+1,Category::unknown);
    if (hints_[id]!=Category::unknown) return hints_[id];
    const auto s=ids_.spelling(id);
    Category hint=Category::value;
    for (std::size_t i=0;i<s.size;++i) {
        if (s.data[i]=='T') { hint=Category::templ; break; }
        if (s.data[i]=='C' || s.data[i]=='Y' || s.data[i]=='E') hint=Category::type;
    }
    hints_[id]=hint; return hint;
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
    const auto& t=peek(offset);
    if (t.kind==PostKind::simple) return specifier(t.simple) || t.simple==SimpleKind::KW_TYPENAME || t.simple==SimpleKind::KW_DECLTYPE;
    if (t.kind==PostKind::identifier) {
        Category c=category(t.identifier); return c==Category::type || c==Category::templ;
    }
    return false;
}
NodeId SyntaxParser::leaf(SyntaxKind k, const PostToken& t) {
    if (t.kind==PostKind::identifier) return tree_.name(k,t.identifier,t.location);
    return tree_.token(k,t.simple,t.location);
}
NodeId SyntaxParser::raw(SyntaxKind k, SimpleKind token) {
    NodeId n=tree_.token(k,token); tree_.nodes[n].payload=SyntaxPayload::raw_token; return n;
}
NodeId SyntaxParser::name(SyntaxKind kind) {
    if (peek().kind!=PostKind::identifier) error("identifier");
    return leaf(kind,take());
}
NodeId SyntaxParser::specs(bool type) {
    NodeId result=tree_.node(type ? SyntaxKind::TypeSpecifiers : SyntaxKind::DeclSpecifiers);
    bool has_type=false;
    while (true) {
        const auto t=peek();
        if (t.kind==PostKind::simple && specifier(t.simple)) {
            if (has_type && builtin(t.simple) && t.simple!=SimpleKind::KW_INT && t.simple!=SimpleKind::KW_LONG && t.simple!=SimpleKind::KW_DOUBLE) break;
            take(); if (builtin(t.simple)) has_type=true;
            SyntaxKind k=type ? (t.simple==SimpleKind::KW_CONST || t.simple==SimpleKind::KW_VOLATILE ? SyntaxKind::CvQualifier : SyntaxKind::TypeSpecifier) : SyntaxKind::DeclSpecifier;
            tree_.append(result,leaf(k,t));
        } else if (!has_type && t.kind==PostKind::identifier && type_start()) {
            take(); has_type=true;
            NodeId n=leaf(type ? SyntaxKind::TypeName : SyntaxKind::DeclSpecifier,t);
            if (!type) tree_.nodes[n].payload=SyntaxPayload::spelling;
            if (!type) {
                auto s=ids_.spelling(t.identifier); tree_.nodes[n].offset=tree_.spellings.size();
                std::string text="TT_IDENTIFIER:"+std::string(s.data,s.size);
                tree_.nodes[n].length=text.size(); tree_.spellings.insert(tree_.spellings.end(),text.begin(),text.end());
            }
            tree_.append(result,n);
        } else break;
    }
    if (!has_type) error("type specifier");
    return result;
}
IdentifierId SyntaxParser::declared_name(NodeId n) const {
    // Only declarator-local edges are visited; no TU scan or recursion.
    while (n) {
        NodeId nested=0;
        for (auto e=tree_.nodes[n].first;e;e=tree_.edges[e].next) {
            NodeId c=tree_.edges[e].child;
            if (tree_.nodes[c].kind==SyntaxKind::Identifier) return tree_.nodes[c].name;
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
            const bool dots=at(SimpleKind::OP_LPAREN) && at(SimpleKind::OP_DOTS,1);
            NodeId d=declarator(dots); tree_.append(p,d); bind(declared_name(d),Category::value);
        }
        if (eat(SimpleKind::OP_ASS)) {
            NodeId a=tree_.node(SyntaxKind::DefaultArgument); tree_.append(a,initializer(true)); tree_.append(p,a);
        }
        tree_.append(result,p);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_RPAREN); return result;
}
NodeId SyntaxParser::declarator(bool abstract, bool allow_name) {
    NodeId result=tree_.node(abstract ? SyntaxKind::AbstractDeclarator : SyntaxKind::Declarator);
    while (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND)) {
        tree_.append(result,leaf(SyntaxKind::Pointer,take()));
        while (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE)) tree_.append(result,leaf(SyntaxKind::CvQualifier,take()));
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    if (allow_name && peek().kind==PostKind::identifier) tree_.append(result,name(SyntaxKind::Identifier));
    else if (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || (peek(1).kind==PostKind::identifier && !type_start(1)))) {
        take(); NodeId nested=tree_.node(SyntaxKind::NestedDeclarator);
        tree_.append(nested,declarator(abstract,allow_name)); require(SimpleKind::OP_RPAREN); tree_.append(result,nested);
    }
    if (eat(SimpleKind::OP_DOTS)) tree_.append(result,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
    while (true) {
        if (eat(SimpleKind::OP_LSQUARE)) {
            NodeId a=tree_.node(SyntaxKind::ArraySuffix);
            if (!at(SimpleKind::OP_RSQUARE)) tree_.append(a,expression(2));
            require(SimpleKind::OP_RSQUARE); tree_.append(result,a);
        } else if (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_RPAREN,1) || at(SimpleKind::OP_DOTS,1) || type_start(1))) {
            tree_.append(result,parameters());
            while (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::KW_NOEXCEPT)) {
                PostToken t=take(); NodeId q=raw(SyntaxKind::FunctionQualifier,t.simple);
                if (t.simple==SimpleKind::KW_NOEXCEPT && eat(SimpleKind::OP_LPAREN)) {
                    tree_.append(q,expression()); require(SimpleKind::OP_RPAREN);
                }
                tree_.append(result,q);
            }
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
NodeId SyntaxParser::type_id() {
    NodeId result=tree_.node(SyntaxKind::TypeId); tree_.append(result,specs(true));
    if (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) || at(SimpleKind::OP_LSQUARE) ||
        (at(SimpleKind::OP_LPAREN) && (at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1) || at(SimpleKind::OP_LAND,1) || type_start(1)))) tree_.append(result,declarator(true,false));
    return result;
}
NodeId SyntaxParser::list(SyntaxKind kind, SimpleKind close) {
    NodeId result=tree_.node(kind);
    while (!at(close)) {
        tree_.append(result,at(SimpleKind::OP_LBRACE) ? initializer() : expression(2));
        if (!eat(SimpleKind::OP_COMMA)) break;
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
    attributes();
    if (eat(SimpleKind::OP_SEMICOLON)) return tree_.node(SyntaxKind::EmptyDeclaration);
    if (eat(SimpleKind::KW_STATIC_ASSERT)) {
        NodeId result=tree_.node(SyntaxKind::StaticAssert); require(SimpleKind::OP_LPAREN);
        tree_.append(result,expression(2)); require(SimpleKind::OP_COMMA);
        auto t=take(); if (t.kind!=PostKind::array) error("string literal");
        tree_.append(result,tree_.text(SyntaxKind::Message,t.source,t.location));
        require(SimpleKind::OP_RPAREN); require(SimpleKind::OP_SEMICOLON); return result;
    }
    if (eat(SimpleKind::KW_USING)) {
        auto id=take(); if (id.kind!=PostKind::identifier) error("alias name");
        require(SimpleKind::OP_ASS); NodeId result=leaf(SyntaxKind::Alias,id);
        tree_.append(result,type_id()); require(SimpleKind::OP_SEMICOLON); bind(id.identifier,Category::type); return result;
    }
    const bool typedef_decl=at(SimpleKind::KW_TYPEDEF);
    NodeId spec=specs(); NodeId result=tree_.node(SyntaxKind::SimpleDeclaration); tree_.append(result,spec);
    if (eat(SimpleKind::OP_SEMICOLON)) return result;
    NodeId init_list=tree_.node(SyntaxKind::InitDeclarators);
    while (true) {
        enter(); // parameters' scope extends through this function's body, not sibling declarations
        NodeId d=declarator(); IdentifierId id=declared_name(d);
        if (!id) error("named declarator");
        if (function_declarator(d) && (at(SimpleKind::OP_LBRACE) || at(SimpleKind::KW_TRY))) {
            tree_.nodes[result].kind=SyntaxKind::FunctionDefinition;
            tree_.append(result,d);
            bind(id,Category::value);
            if (eat(SimpleKind::KW_TRY)) { NodeId body=try_statement(); tree_.nodes[body].kind=SyntaxKind::FunctionTry; tree_.append(result,body); }
            else tree_.append(result,compound());
            leave(); bind(id,Category::value); return result;
        }
        leave(); bind(id,typedef_decl ? Category::type : Category::value);
        NodeId init=tree_.node(SyntaxKind::InitDeclarator); tree_.append(init,d);
        if (eat(SimpleKind::OP_ASS)) tree_.append(init,initializer(true));
        else if (at(SimpleKind::OP_LPAREN)) tree_.append(init,initializer());
        else if (at(SimpleKind::OP_LBRACE)) {
            NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(init,i);
        }
        tree_.append(init_list,init);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_SEMICOLON); tree_.append(result,init_list); return result;
}
NodeId SyntaxParser::parse() {
    NodeId root=tree_.node(SyntaxKind::TranslationUnit);
    while (peek().kind!=PostKind::eof) tree_.append(root,declaration());
    return root;
}
}
