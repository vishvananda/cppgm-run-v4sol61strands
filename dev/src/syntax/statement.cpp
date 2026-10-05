#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::compound() {
    if (expected_) return 0;
    if (!require(SimpleKind::OP_LBRACE)) { return 0; } enter(); NodeId result=tree_.node(SyntaxKind::Compound); tree_.nodes[result].scope=active_.back();
    while (!expected_ && (!at(SimpleKind::OP_RBRACE))) {
        if (peek().kind==PostKind::eof) return error("closing brace");
        tree_.append(result,statement());
    }
    take(); leave(); return result;
}
NodeId SyntaxParser::condition() {
    if (expected_) return 0;
    NodeId result=tree_.node(SyntaxKind::Condition);
    // A type followed by a name starts a condition declaration, not a call.
    if (type_start() && (peek(1).kind==PostKind::identifier || at(SimpleKind::OP_STAR,1) || at(SimpleKind::OP_AMP,1))) {
        NodeId d=tree_.node(SyntaxKind::ConditionDeclaration); tree_.append(d,specs());
        NodeId decl=declarator(); tree_.append(d,decl); bind(declared_name(decl),Category::value);
        if (eat(SimpleKind::OP_ASS)) tree_.append(d,initializer(true));
        else if (at(SimpleKind::OP_LBRACE)) { NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(d,i); }
        else return error("condition initializer");
        tree_.append(result,d);
    } else tree_.append(result,expression());
    return result;
}
NodeId SyntaxParser::for_statement() {
    if (expected_) return 0;
    if (!require(SimpleKind::OP_LPAREN)) { return 0; } enter(); NodeId result=tree_.node(SyntaxKind::For);
    NodeId init=tree_.node(SyntaxKind::ForInit);
    if (type_start()) {
        // Factor the shared declaration prefix once; no token rollback or grammar replay.
        const bool is_typedef=at(SimpleKind::KW_TYPEDEF);
        NodeId s=specs(); NodeId d=declarator(); bind(declared_name(d),is_typedef ? Category::type : Category::value);
        if (eat(SimpleKind::OP_COLON)) {
            tree_.nodes[result].kind=SyntaxKind::RangeFor;
            tree_.nodes[init].kind=SyntaxKind::RangeDeclaration; tree_.append(init,s); tree_.append(init,d); tree_.append(result,init);
            NodeId range=tree_.node(SyntaxKind::RangeInitializer);
            tree_.append(range,at(SimpleKind::OP_LBRACE) ? initializer() : expression()); tree_.append(result,range);
            if (!require(SimpleKind::OP_RPAREN)) { return 0; } tree_.append(result,scoped_statement()); leave(); return result;
        }
        NodeId decl=tree_.node(SyntaxKind::SimpleDeclaration); tree_.append(decl,s);
        NodeId ds=tree_.node(SyntaxKind::InitDeclarators);
        while (!expected_ && (true)) {
            NodeId item=tree_.node(SyntaxKind::InitDeclarator); tree_.append(item,d);
            if (eat(SimpleKind::OP_ASS)) tree_.append(item,initializer(true));
            else if (at(SimpleKind::OP_LPAREN)) tree_.append(item,initializer());
            else if (at(SimpleKind::OP_LBRACE)) { NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(item,i); }
            tree_.append(ds,item);
            if (!eat(SimpleKind::OP_COMMA)) break;
            d=declarator(); bind(declared_name(d),is_typedef ? Category::type : Category::value);
        }
        if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } tree_.append(decl,ds); tree_.append(init,decl);
    } else {
        NodeId e=tree_.node(SyntaxKind::ExpressionStatement);
        if (!at(SimpleKind::OP_SEMICOLON)) tree_.append(e,expression());
        if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } tree_.append(init,e);
    }
    tree_.append(result,init);
    if (!at(SimpleKind::OP_SEMICOLON)) tree_.append(result,condition());
    if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
    if (!at(SimpleKind::OP_RPAREN)) { NodeId i=tree_.node(SyntaxKind::Iteration); tree_.append(i,expression()); tree_.append(result,i); }
    if (!require(SimpleKind::OP_RPAREN)) { return 0; } tree_.append(result,scoped_statement()); leave(); return result;
}
NodeId SyntaxParser::try_statement(NodeId result) {
    if (expected_) return 0;
    if (!result) result=tree_.node(SyntaxKind::Try);
    tree_.append(result,compound());
    if (!at(SimpleKind::KW_CATCH)) return error("catch");
    while (!expected_ && (eat(SimpleKind::KW_CATCH))) {
        enter(); NodeId handler=tree_.node(SyntaxKind::Handler); if (!require(SimpleKind::OP_LPAREN)) { return 0; }
        NodeId ex=tree_.node(SyntaxKind::ExceptionDeclaration);
        if (eat(SimpleKind::OP_DOTS)) tree_.append(ex,raw(SyntaxKind::Ellipsis,SimpleKind::OP_DOTS));
        else {
            tree_.append(ex,specs());
            if (!at(SimpleKind::OP_RPAREN)) { NodeId d=declarator(); tree_.append(ex,d); bind(declared_name(d),Category::value); }
        }
        if (!require(SimpleKind::OP_RPAREN)) { return 0; } tree_.append(handler,ex); tree_.append(handler,compound());
        tree_.append(result,handler); leave();
    }
    return result;
}
NodeId SyntaxParser::scoped_statement() {
    if (expected_) return 0;
    enter(); NodeId result=statement(); leave(); return result;
}
NodeId SyntaxParser::statement() {
    if (expected_) return 0;
    if (at(SimpleKind::OP_LBRACE)) return compound();
    if (eat(SimpleKind::KW_TRY)) return try_statement();
    if (eat(SimpleKind::KW_IF)) {
        enter(); NodeId result=tree_.node(SyntaxKind::If); if (!require(SimpleKind::OP_LPAREN)) { return 0; }
        tree_.append(result,condition()); if (!require(SimpleKind::OP_RPAREN)) { return 0; }
        NodeId then=tree_.node(SyntaxKind::Then); tree_.append(then,scoped_statement()); tree_.append(result,then);
        if (eat(SimpleKind::KW_ELSE)) { NodeId e=tree_.node(SyntaxKind::Else); tree_.append(e,scoped_statement()); tree_.append(result,e); }
        leave(); return result;
    }
    if (at(SimpleKind::KW_WHILE) || at(SimpleKind::KW_SWITCH)) {
        auto t=take(); enter(); NodeId result=tree_.node(t.simple==SimpleKind::KW_WHILE ? SyntaxKind::While : SyntaxKind::Switch);
        if (!require(SimpleKind::OP_LPAREN)) { return 0; } tree_.append(result,condition()); if (!require(SimpleKind::OP_RPAREN)) { return 0; }
        tree_.append(result,scoped_statement()); leave(); return result;
    }
    if (eat(SimpleKind::KW_DO)) {
        NodeId result=tree_.node(SyntaxKind::Do); tree_.append(result,scoped_statement()); if (!require(SimpleKind::KW_WHILE)) { return 0; }
        if (!require(SimpleKind::OP_LPAREN)) { return 0; } tree_.append(result,condition()); if (!require(SimpleKind::OP_RPAREN)) { return 0; } if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
    }
    if (eat(SimpleKind::KW_FOR)) return for_statement();
    if (eat(SimpleKind::KW_THROW)) {
        NodeId result=tree_.node(SyntaxKind::ThrowStatement);
        if (!at(SimpleKind::OP_SEMICOLON)) tree_.append(result,expression(2));
        if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
    }
    if (eat(SimpleKind::KW_RETURN)) {
        NodeId result=tree_.node(SyntaxKind::Return);
        if (!at(SimpleKind::OP_SEMICOLON)) tree_.append(result,at(SimpleKind::OP_LBRACE) ? initializer() : expression());
        if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
    }
    if (at(SimpleKind::KW_BREAK) || at(SimpleKind::KW_CONTINUE)) {
        auto t=take(); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
        return tree_.node(t.simple==SimpleKind::KW_BREAK ? SyntaxKind::Break : SyntaxKind::Continue);
    }
    if (eat(SimpleKind::KW_GOTO)) { NodeId result=name(SyntaxKind::Goto); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result; }
    if (eat(SimpleKind::KW_CASE)) {
        NodeId result=tree_.node(SyntaxKind::Case); tree_.append(result,expression(2)); if (!require(SimpleKind::OP_COLON)) { return 0; } tree_.append(result,statement()); return result;
    }
    if (eat(SimpleKind::KW_DEFAULT)) { if (!require(SimpleKind::OP_COLON)) { return 0; } NodeId result=tree_.node(SyntaxKind::Default); tree_.append(result,statement()); return result; }
    if (peek().kind==PostKind::identifier && at(SimpleKind::OP_COLON,1)) {
        NodeId result=name(SyntaxKind::Label); take(); tree_.append(result,statement()); return result;
    }
    if (type_start() && at(SimpleKind::OP_LPAREN,1)) return ambiguous_statement();
    if (type_start() || at(SimpleKind::KW_USING) || at(SimpleKind::KW_STATIC_ASSERT)) return declaration();
    NodeId result=tree_.node(SyntaxKind::ExpressionStatement);
    if (!at(SimpleKind::OP_SEMICOLON)) tree_.append(result,expression());
    if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
}
}
