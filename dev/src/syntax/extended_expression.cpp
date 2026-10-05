#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::allocation() {
    bool global=eat(SimpleKind::OP_COLON2);
    bool deleting=eat(SimpleKind::KW_DELETE);
    NodeId result=tree_.node(deleting ? SyntaxKind::Delete : SyntaxKind::New);
    if (global) tree_.append(result,tree_.node(SyntaxKind::GlobalScope));
    if (deleting) {
        if (eat(SimpleKind::OP_LSQUARE)) { require(SimpleKind::OP_RSQUARE); tree_.append(result,tree_.node(SyntaxKind::ArrayDelete)); }
        tree_.append(result,unary()); return result;
    }
    require(SimpleKind::KW_NEW);
    if (at(SimpleKind::OP_LPAREN) && !type_start(1)) {
        take(); NodeId p=tree_.node(SyntaxKind::Placement);
        tree_.append(p,list(SyntaxKind::ParenArguments,SimpleKind::OP_RPAREN)); tree_.append(result,p);
    }
    if (eat(SimpleKind::OP_LPAREN)) { tree_.append(result,type_id()); require(SimpleKind::OP_RPAREN); }
    else tree_.append(result,type_id(true));
    if (at(SimpleKind::OP_LPAREN)) tree_.append(result,initializer());
    else if (at(SimpleKind::OP_LBRACE)) {
        NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(result,i);
    }
    return result;
}
NodeId SyntaxParser::lambda() {
    require(SimpleKind::OP_LSQUARE); enter();
    NodeId result=tree_.node(SyntaxKind::Lambda);
    // Capture syntax is retained as ordered typed leaves; rendered introducer
    // text is just the PA5 view, not the later capture/lifetime representation.
    NodeId intro=tree_.node(SyntaxKind::LambdaIntroducer);
    while (!at(SimpleKind::OP_RSQUARE)) {
        NodeId c=tree_.node(SyntaxKind::Capture);
        if (eat(SimpleKind::OP_AMP)) { tree_.nodes[c].payload=SyntaxPayload::raw_token; tree_.nodes[c].token=SimpleKind::OP_AMP; }
        else if (eat(SimpleKind::OP_ASS)) { tree_.nodes[c].payload=SyntaxPayload::raw_token; tree_.nodes[c].token=SimpleKind::OP_ASS; }
        if (peek().kind==PostKind::identifier) tree_.append(c,name(SyntaxKind::Identifier));
        else if (eat(SimpleKind::KW_THIS)) tree_.append(c,raw(SyntaxKind::This,SimpleKind::KW_THIS));
        else if (tree_.nodes[c].payload==SyntaxPayload::none) error("lambda capture");
        if (eat(SimpleKind::OP_DOTS)) tree_.append(c,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
        tree_.append(intro,c);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_RSQUARE); tree_.append(result,intro);
    if (at(SimpleKind::OP_LPAREN) || at(SimpleKind::KW_MUTABLE) || at(SimpleKind::KW_NOEXCEPT) || at(SimpleKind::OP_ARROW)) {
        NodeId d=tree_.node(SyntaxKind::LambdaDeclarator);
        if (at(SimpleKind::OP_LPAREN)) tree_.append(d,parameters());
        if (eat(SimpleKind::KW_MUTABLE)) tree_.append(d,tree_.token(SyntaxKind::Mutable,SimpleKind::KW_MUTABLE));
        if (eat(SimpleKind::KW_NOEXCEPT)) {
            NodeId q=tree_.node(SyntaxKind::Noexcept);
            if (eat(SimpleKind::OP_LPAREN)) { tree_.append(q,expression()); require(SimpleKind::OP_RPAREN); }
            tree_.append(d,q);
        }
        if (eat(SimpleKind::OP_ARROW)) { NodeId r=tree_.node(SyntaxKind::TrailingReturn); tree_.append(r,type_id()); tree_.append(d,r); }
        tree_.append(result,d);
    }
    tree_.append(result,compound()); leave(); return result;
}
NodeId SyntaxParser::attributes(NodeId owner) {
    NodeId result=0;
    auto capture=[&]() {
        auto t=take();
        if (t.literal_node) tree_.append(result,t.literal_node);
        // Attribute tokens are syntax-only; literals still keep their unique
        // graph owner, typed bytes and source identity even when the view omits them.
    };
    while (true) {
        // Attributes/alignment are omitted from the PA5 view, but their balanced
        // source extent is consumed once. No owning spelling or grammar replay.
        if (at(SimpleKind::KW_ALIGNAS) || (peek().kind==PostKind::identifier && peek().identifier==attribute_id_)) {
            if (!result) result=tree_.node(SyntaxKind::Attribute);
            take(); require(SimpleKind::OP_LPAREN); unsigned depth=1;
            while (depth) {
                if (peek().kind==PostKind::eof) error("attribute closing parenthesis");
                if (at(SimpleKind::OP_LPAREN)) ++depth;
                if (at(SimpleKind::OP_RPAREN)) --depth;
                capture();
            }
            continue;
        }
        if (!at(SimpleKind::OP_LSQUARE) || !at(SimpleKind::OP_LSQUARE,1)) break;
        if (!result) result=tree_.node(SyntaxKind::Attribute);
        take(); take();
        std::vector<SimpleKind> close;
        while (true) {
            if (close.empty() && at(SimpleKind::OP_RSQUARE) && at(SimpleKind::OP_RSQUARE,1)) { take(); take(); break; }
            if (peek().kind==PostKind::eof || peek().kind==PostKind::invalid) error("balanced attribute");
            auto t=take(); if (t.literal_node) tree_.append(result,t.literal_node);
            if (t.kind!=PostKind::simple) continue;
            if (t.simple==SimpleKind::OP_LPAREN) close.push_back(SimpleKind::OP_RPAREN);
            else if (t.simple==SimpleKind::OP_LBRACE) close.push_back(SimpleKind::OP_RBRACE);
            else if (t.simple==SimpleKind::OP_LSQUARE) close.push_back(SimpleKind::OP_RSQUARE);
            else if (t.simple==SimpleKind::OP_RPAREN || t.simple==SimpleKind::OP_RBRACE || t.simple==SimpleKind::OP_RSQUARE) {
                if (close.empty() || close.back()!=t.simple) error("balanced attribute delimiter");
                close.pop_back();
            }
        }
    }
    if (owner && result) tree_.append(owner,result);
    return result;
}
}
