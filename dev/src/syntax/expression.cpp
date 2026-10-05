#include "syntax/parser.h"
namespace cppgm {
namespace {
int precedence(SimpleKind k) {
    switch (k) {
    case SimpleKind::OP_COMMA: return 1;
    case SimpleKind::OP_ASS: case SimpleKind::OP_PLUSASS: case SimpleKind::OP_MINUSASS:
    case SimpleKind::OP_STARASS: case SimpleKind::OP_DIVASS: case SimpleKind::OP_MODASS:
    case SimpleKind::OP_XORASS: case SimpleKind::OP_BANDASS: case SimpleKind::OP_BORASS:
    case SimpleKind::OP_LSHIFTASS: case SimpleKind::OP_RSHIFTASS: return 2;
    case SimpleKind::OP_QMARK: return 3;
    case SimpleKind::OP_LOR: return 4;
    case SimpleKind::OP_LAND: return 5;
    case SimpleKind::OP_BOR: return 6;
    case SimpleKind::OP_XOR: return 7;
    case SimpleKind::OP_AMP: return 8;
    case SimpleKind::OP_EQ: case SimpleKind::OP_NE: return 9;
    case SimpleKind::OP_LT: case SimpleKind::OP_GT: case SimpleKind::OP_LE: case SimpleKind::OP_GE: return 10;
    case SimpleKind::OP_LSHIFT: case SimpleKind::OP_RSHIFT: return 11;
    case SimpleKind::OP_PLUS: case SimpleKind::OP_MINUS: return 12;
    case SimpleKind::OP_STAR: case SimpleKind::OP_DIV: case SimpleKind::OP_MOD: return 13;
    case SimpleKind::OP_DOTSTAR: case SimpleKind::OP_ARROWSTAR: return 14;
    default: return 0;
    }
}
}
NodeId SyntaxParser::expression(int minimum) {
    NodeId left=unary();
    while (peek().kind==PostKind::simple) {
        SimpleKind op=peek().simple; int p=precedence(op);
        if (p<minimum) break;
        auto t=take();
        if (op==SimpleKind::OP_QMARK) {
            NodeId result=tree_.node(SyntaxKind::Conditional,t.location); tree_.append(result,left);
            tree_.append(result,expression()); require(SimpleKind::OP_COLON);
            tree_.append(result,expression(2)); left=result;
        } else {
            SyntaxKind kind=p==1 ? SyntaxKind::Comma : p==2 ? SyntaxKind::Assignment : SyntaxKind::Binary;
            NodeId result=p==1 ? tree_.node(kind,t.location) : leaf(kind,t);
            tree_.append(result,left);
            tree_.append(result,p==2 && at(SimpleKind::OP_LBRACE) ? initializer() : expression(p+(p!=2)));
            left=result;
        }
    }
    return left;
}
NodeId SyntaxParser::unary() {
    if (at(SimpleKind::KW_NEW) || at(SimpleKind::KW_DELETE) ||
        (at(SimpleKind::OP_COLON2) && (at(SimpleKind::KW_NEW,1) || at(SimpleKind::KW_DELETE,1)))) return allocation();
    if (at(SimpleKind::OP_INC) || at(SimpleKind::OP_DEC) || at(SimpleKind::OP_PLUS) || at(SimpleKind::OP_MINUS) || at(SimpleKind::OP_LNOT) || at(SimpleKind::OP_COMPL) || at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP)) {
        NodeId result=leaf(SyntaxKind::Unary,take()); tree_.append(result,unary()); return result;
    }
    if (eat(SimpleKind::KW_THROW)) {
        NodeId result=tree_.node(SyntaxKind::Throw);
        if (!at(SimpleKind::OP_SEMICOLON) && !at(SimpleKind::OP_RPAREN) && !at(SimpleKind::OP_COLON)) tree_.append(result,expression(2));
        return result;
    }
    if (at(SimpleKind::KW_STATIC_CAST) || at(SimpleKind::KW_DYNAMIC_CAST) || at(SimpleKind::KW_CONST_CAST) || at(SimpleKind::KW_REINTERPET_CAST)) {
        NodeId result=leaf(SyntaxKind::Cast,take()); require(SimpleKind::OP_LT);
        tree_.append(result,type_id()); require(SimpleKind::OP_GT); require(SimpleKind::OP_LPAREN);
        tree_.append(result,expression()); require(SimpleKind::OP_RPAREN); return postfix(result);
    }
    if (at(SimpleKind::KW_SIZEOF) || at(SimpleKind::KW_ALIGNOF) || at(SimpleKind::KW_TYPEID) || at(SimpleKind::KW_NOEXCEPT)) {
        PostToken t=take();
        if (t.simple==SimpleKind::KW_SIZEOF && eat(SimpleKind::OP_DOTS)) {
            require(SimpleKind::OP_LPAREN); NodeId result=name(SyntaxKind::SizeofPack); require(SimpleKind::OP_RPAREN); return result;
        }
        NodeId result=t.simple==SimpleKind::KW_SIZEOF ? tree_.node(SyntaxKind::Sizeof,t.location) : leaf(SyntaxKind::Trait,t);
        if (eat(SimpleKind::OP_LPAREN)) {
            if (type_start()) tree_.append(result,type_id());
            else tree_.append(result,expression());
            require(SimpleKind::OP_RPAREN);
        } else tree_.append(result,unary());
        return result;
    }
    if (at(SimpleKind::OP_LPAREN) && type_start(1)) {
        // Type-led parenthesized regions share type-id parsing with casts/traits.
        take(); NodeId type=type_id(); require(SimpleKind::OP_RPAREN);
        NodeId result=tree_.token(SyntaxKind::Cast,SimpleKind::OP_LPAREN);
        tree_.append(result,type); tree_.append(result,unary()); return result;
    }
    return postfix(primary());
}
NodeId SyntaxParser::primary() {
    if (at(SimpleKind::OP_LSQUARE)) return lambda();
    const PostToken t=take();
    if (t.kind==PostKind::identifier) return leaf(SyntaxKind::IdExpression,t);
    if (t.kind!=PostKind::simple && t.kind!=PostKind::invalid && t.kind!=PostKind::eof) return t.numeric_prefix;
    if (t.kind!=PostKind::simple) error("expression");
    if (t.simple==SimpleKind::KW_TRUE || t.simple==SimpleKind::KW_FALSE || t.simple==SimpleKind::KW_NULLPTR) return leaf(SyntaxKind::KeywordLiteral,t);
    if (t.simple==SimpleKind::KW_THIS) return tree_.node(SyntaxKind::This,t.location);
    if (t.simple==SimpleKind::OP_LPAREN) {
        NodeId result=tree_.node(SyntaxKind::Parenthesized,t.location); tree_.append(result,expression()); require(SimpleKind::OP_RPAREN); return result;
    }
    if (builtin(t.simple)) {
        NodeId result=tree_.node(SyntaxKind::Call,t.location); NodeId type=raw(SyntaxKind::IdExpression,t.simple); tree_.append(result,type);
        if (eat(SimpleKind::OP_LPAREN)) tree_.append(result,list(SyntaxKind::ParenArguments,SimpleKind::OP_RPAREN));
        else if (eat(SimpleKind::OP_LBRACE)) tree_.append(result,list(SyntaxKind::BracedInit,SimpleKind::OP_RBRACE));
        else error("conversion initializer");
        return result;
    }
    error("primary expression");
}
NodeId SyntaxParser::postfix(NodeId left) {
    while (true) {
        if (eat(SimpleKind::OP_LPAREN)) {
            NodeId result=tree_.node(SyntaxKind::Call); tree_.append(result,left);
            tree_.append(result,list(SyntaxKind::Arguments,SimpleKind::OP_RPAREN)); left=result;
        } else if (eat(SimpleKind::OP_LSQUARE)) {
            NodeId result=tree_.node(SyntaxKind::Subscript); tree_.append(result,left);
            tree_.append(result,expression()); require(SimpleKind::OP_RSQUARE); left=result;
        } else if (at(SimpleKind::OP_DOT) || at(SimpleKind::OP_ARROW)) {
            NodeId result=leaf(SyntaxKind::Member,take()); tree_.append(result,left);
            tree_.append(result,name(SyntaxKind::Identifier)); left=result;
        } else if (at(SimpleKind::OP_INC) || at(SimpleKind::OP_DEC)) {
            NodeId result=leaf(SyntaxKind::Postfix,take()); tree_.append(result,left); left=result;
        } else break;
    }
    return left;
}
}
