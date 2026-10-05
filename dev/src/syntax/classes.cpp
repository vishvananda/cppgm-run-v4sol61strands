#include "syntax/parser.h"
#include <algorithm>
namespace cppgm {
NodeId SyntaxParser::class_name(SyntaxKind kind) {
    if (!eat(SimpleKind::KW_DECLTYPE)) return qualified(kind);
    NodeId result=tree_.node(kind); tree_.nodes[result].is_decltype=true;
    require(SimpleKind::OP_LPAREN); tree_.append(result,expression()); require(SimpleKind::OP_RPAREN);
    return result;
}
bool SyntaxParser::special_start() {
    if (at(SimpleKind::OP_COMPL)) return true;
    if (at(SimpleKind::KW_OPERATOR)) return type_start(1);
    if (peek().kind!=PostKind::identifier) return false;
    if (!classes_.empty() && peek().identifier==classes_.back().name && at(SimpleKind::OP_LPAREN,1)) return true;
    if (lookahead_.front().name_node) {
        NodeId n=lookahead_.front().name_node;
        if (tree_.nodes[n].destructor || tree_.nodes[n].operator_conversion) return true;
        // The qualified name is already factored, never render it as a lookup key.
        NodeId prev=n, last=n;
        for (auto e=tree_.nodes[n].first;e;e=tree_.edges[e].next) {
            NodeId c=tree_.edges[e].child;
            if (tree_.nodes[c].kind==SyntaxKind::Identifier) { prev=last; last=c; }
        }
        return last!=n && (tree_.nodes[last].destructor || tree_.nodes[last].operator_conversion || tree_.nodes[last].name==tree_.nodes[prev].name) && at(SimpleKind::OP_LPAREN,1);
    }
    return false;
}
NodeId SyntaxParser::class_specifier() {
    auto key=take();
    NodeId result=tree_.node(SyntaxKind::Class,key.location); attributes(result);
    IdentifierId id=0;
    if (peek().kind==PostKind::identifier) {
        // Class heads are identifiers at this stage. Template-id heads are owned
        // by template parsing, not invented through spelling-based heuristics.
        auto token=take(); id=token.identifier;
        tree_.nodes[result].payload=SyntaxPayload::identifier; tree_.nodes[result].name=id;
    }
    tree_.append(result,leaf(SyntaxKind::ClassKey,key)); attributes(result);
    SyntaxScopeId scope=0;
    if (id) {
        auto existing=tree_.scopes[active_.back()].names.find(id);
        if (existing && existing->second.qualifier_category==Category::type) scope=existing->second.qualifier;
    }
    if (!scope) scope=create_scope(active_.back());
    bind(id,Category::type,scope); tree_.nodes[result].scope=scope;
    if (peek().kind==PostKind::identifier && final_id_==peek().identifier) take();
    if (eat(SimpleKind::OP_COLON)) {
        NodeId bases=tree_.node(SyntaxKind::BaseClause);
        do {
            NodeId b=tree_.node(SyntaxKind::Base); attributes(b);
            while (at(SimpleKind::KW_VIRTUAL) || at(SimpleKind::KW_PUBLIC) || at(SimpleKind::KW_PROTECTED) || at(SimpleKind::KW_PRIVATE)) {
                auto t=take(); tree_.append(b,leaf(t.simple==SimpleKind::KW_VIRTUAL ? SyntaxKind::Virtual : SyntaxKind::Access,t));
            }
            NodeId name=class_name(SyntaxKind::BaseName); tree_.append(b,name);
            // Indexed base edges allow inherited categories without a TU scan.
            if (tree_.nodes[name].resolved_scope) tree_.scopes[scope].bases.push_back(tree_.nodes[name].resolved_scope);
            if (eat(SimpleKind::OP_DOTS)) tree_.append(b,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
            tree_.append(bases,b);
        } while (eat(SimpleKind::OP_COMMA));
        tree_.append(result,bases);
    }
    if (!eat(SimpleKind::OP_LBRACE)) {
        if (!id) error("class name or body");
        tree_.nodes[result].kind=SyntaxKind::ClassForward; return result;
    }
    tree_.nodes[result].complete_definition=true;
    const std::size_t first=deferred_.size();
    enter(scope); bind(id,Category::type,scope); classes_.push_back({id,scope});
    while (!at(SimpleKind::OP_RBRACE)) {
        if (peek().kind==PostKind::eof) error("closing class brace");
        if (at(SimpleKind::KW_PUBLIC) || at(SimpleKind::KW_PRIVATE) || at(SimpleKind::KW_PROTECTED)) {
            auto t=take(); require(SimpleKind::OP_COLON); tree_.append(result,leaf(SyntaxKind::Access,t));
        } else tree_.append(result,declaration());
    }
    take(); classes_.pop_back(); leave();
    // Nested classes share the enclosing complete-class boundary.
    if (classes_.empty()) finish_bodies(first);
    return result;
}
NodeId SyntaxParser::ctor_initializer() {
    require(SimpleKind::OP_COLON); NodeId result=tree_.node(SyntaxKind::CtorInitializer);
    do {
        NodeId m=tree_.node(SyntaxKind::MemInitializer);
        tree_.append(m,class_name(SyntaxKind::MemInitializerId));
        if (eat(SimpleKind::OP_LPAREN)) tree_.append(m,list(SyntaxKind::ParenArguments,SimpleKind::OP_RPAREN));
        else if (at(SimpleKind::OP_LBRACE)) tree_.append(m,initializer());
        else error("member initializer arguments");
        if (eat(SimpleKind::OP_DOTS)) tree_.append(m,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
        tree_.append(result,m);
    } while (eat(SimpleKind::OP_COMMA));
    return result;
}
NodeId SyntaxParser::function_body(NodeId owner, bool ready) {
    if (!classes_.empty() && !ready) { defer_body(owner); return owner; }
    bool function_try=eat(SimpleKind::KW_TRY);
    NodeId body=function_try ? tree_.node(SyntaxKind::FunctionTry) : owner;
    if (at(SimpleKind::OP_COLON)) tree_.append(body,ctor_initializer());
    if (function_try) tree_.append(owner,try_statement(body));
    else tree_.append(owner,compound());
    return owner;
}
NodeId SyntaxParser::special_member(NodeId spec, NodeId parsed_name) {
    NodeId result=tree_.node(SyntaxKind::SpecialDeclaration);
    if (spec) {
        tree_.nodes[spec].kind=SyntaxKind::MemberSpecifiers;
        for (auto e=tree_.nodes[spec].first;e;e=tree_.edges[e].next) {
            auto& n=tree_.nodes[tree_.edges[e].child]; n.kind=SyntaxKind::MemberSpecifier;
            if (n.token==SimpleKind::KW_EXPLICIT) n.payload=SyntaxPayload::raw_token;
        }
        tree_.append(result,spec);
    }
    enter(); NodeId d=tree_.node(SyntaxKind::Declarator); tree_.nodes[d].scope=active_.back();
    NodeId n=parsed_name ? parsed_name : qualified(SyntaxKind::Identifier); tree_.append(d,n);
    if (tree_.nodes[n].qualifier_scope) tree_.scopes[active_.back()].parent=tree_.nodes[n].qualifier_scope;
    // Constructor/destructor/conversion names have no return type.
    if (!at(SimpleKind::OP_LPAREN)) error("special member parameter clause");
    suffixes(d); tree_.nodes[d].scope=active_.back(); tree_.append(result,d);
    if (at(SimpleKind::OP_COLON) || at(SimpleKind::OP_LBRACE) || at(SimpleKind::KW_TRY)) {
        tree_.nodes[result].kind=SyntaxKind::SpecialDefinition; function_body(result);
    } else {
        if (eat(SimpleKind::OP_ASS)) {
            if (at(SimpleKind::KW_DEFAULT) || at(SimpleKind::KW_DELETE)) { auto t=take(); NodeId init=tree_.node(SyntaxKind::Initializer); tree_.append(init,raw(SyntaxKind::SpecialInitializer,t.simple)); tree_.append(result,init); }
            else tree_.append(result,initializer(true));
        }
        require(SimpleKind::OP_SEMICOLON);
    }
    leave(); return result;
}
void SyntaxParser::defer_body(NodeId owner) {
    DeferredBody body; body.owner=owner; body.scope=active_.back();
    auto capture=[&]() {
        auto t=take();
        body.tokens.push_back({t.kind,t.simple,t.identifier,t.range,t.location,t.literal_node});
    };
    auto balanced=[&](SimpleKind open, SimpleKind close) {
        if (!at(open)) error("deferred region opener");
        unsigned depth=0;
        do {
            if (peek().kind==PostKind::eof) error("deferred region closer");
            if (at(open)) ++depth;
            if (at(close)) --depth;
            capture();
        } while (depth);
    };
    const bool function_try=at(SimpleKind::KW_TRY);
    if (function_try) capture();
    if (at(SimpleKind::OP_COLON)) {
        capture();
        do {
            if (at(SimpleKind::KW_DECLTYPE)) { capture(); balanced(SimpleKind::OP_LPAREN,SimpleKind::OP_RPAREN); }
            // Retain a mem-initializer-id; its grammar runs only at completion.
            while (!at(SimpleKind::OP_LPAREN) && !at(SimpleKind::OP_LBRACE)) {
                if (peek().kind==PostKind::eof || at(SimpleKind::OP_SEMICOLON)) error("member initializer");
                capture();
            }
            if (at(SimpleKind::OP_LPAREN)) balanced(SimpleKind::OP_LPAREN,SimpleKind::OP_RPAREN);
            else balanced(SimpleKind::OP_LBRACE,SimpleKind::OP_RBRACE);
            if (at(SimpleKind::OP_DOTS)) capture();
            if (!at(SimpleKind::OP_COMMA)) break;
            capture();
        } while (true);
    }
    balanced(SimpleKind::OP_LBRACE,SimpleKind::OP_RBRACE);
    if (function_try) {
        if (!at(SimpleKind::KW_CATCH)) error("function try handler");
        while (at(SimpleKind::KW_CATCH)) {
            capture(); balanced(SimpleKind::OP_LPAREN,SimpleKind::OP_RPAREN);
            balanced(SimpleKind::OP_LBRACE,SimpleKind::OP_RBRACE);
        }
    }
    deferred_.push_back(std::move(body));
}
void SyntaxParser::defer_expression(NodeId owner, DeferredKind kind, SimpleKind close) {
    DeferredBody region; region.owner=owner; region.scope=active_.back(); region.kind=kind;
    if (tree_.nodes[owner].kind==SyntaxKind::DefaultArgument) {
        // One shared-index overlay, not a chain of one scope per parameter.
        // Crossing it during lookup limits existing parameter declarations;
        // scopes created inside the default are encountered before this limit.
        region.scope=create_scope(region.scope);
        tree_.scopes[region.scope].parameter_prefix=true;
        tree_.scopes[region.scope].parameter_limit=parameter_order_;
    }
    tree_.nodes[owner].scope=region.scope;
    std::vector<SimpleKind> delimiters;
    while (true) {
        if (delimiters.empty() && (at(close) || (kind!=DeferredKind::expression && at(SimpleKind::OP_COMMA)))) break;
        if (peek().kind==PostKind::eof) error("complete-class expression boundary");
        auto t=take();
        if (t.kind==PostKind::simple) {
            if (t.simple==SimpleKind::OP_LPAREN) delimiters.push_back(SimpleKind::OP_RPAREN);
            else if (t.simple==SimpleKind::OP_LBRACE) delimiters.push_back(SimpleKind::OP_RBRACE);
            else if (t.simple==SimpleKind::OP_LSQUARE) delimiters.push_back(SimpleKind::OP_RSQUARE);
            else if (t.simple==SimpleKind::OP_RPAREN || t.simple==SimpleKind::OP_RBRACE || t.simple==SimpleKind::OP_RSQUARE) {
                if (delimiters.empty() || delimiters.back()!=t.simple) error("complete-class expression delimiter");
                delimiters.pop_back();
            }
        }
        region.tokens.push_back({t.kind,t.simple,t.identifier,t.range,t.location,t.literal_node});
    }
    if (region.tokens.empty()) error("complete-class expression");
    deferred_.push_back(std::move(region));
}
void SyntaxParser::finish_bodies(std::size_t first) {
    auto continuation=std::move(lookahead_);
    auto outer_active=std::move(active_);
    const auto* outer_input=deferred_input_; auto outer_position=deferred_position_;
    // Detach the queue before parsing: local classes may complete their own
    // regions without invalidating an outer input or iterating moved entries.
    std::vector<DeferredBody> regions;
    for (std::size_t i=first;i<deferred_.size();++i) regions.push_back(std::move(deferred_[i]));
    deferred_.resize(first);
    for (auto& region:regions) {
        active_.clear();
        for (SyntaxScopeId s=region.scope;s;s=tree_.scopes[s].parent) active_.push_back(s);
        std::reverse(active_.begin(),active_.end());
        deferred_input_=&region.tokens; deferred_position_=0; lookahead_.clear();
        if (region.kind==DeferredKind::body) function_body(region.owner,true);
        else if (region.kind==DeferredKind::expression) tree_.append(region.owner,expression());
        else tree_.append(region.owner,initializer(region.kind==DeferredKind::equal_initializer));
        if (peek().kind!=PostKind::eof) error("end of complete-class region");
    }
    deferred_input_=outer_input; deferred_position_=outer_position;
    active_=std::move(outer_active); lookahead_=std::move(continuation);
}
}
