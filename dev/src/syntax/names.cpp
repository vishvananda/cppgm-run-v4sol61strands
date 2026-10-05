#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::qualified(SyntaxKind kind) {
    // Every name component has compact identity. The joined leaf is dump-only.
    bool global=eat(SimpleKind::OP_COLON2);
    NodeId result=0;
    if (eat(SimpleKind::KW_OPERATOR)) {
        if (peek().kind==PostKind::array) {
            auto empty=take();
            const auto& literal=tree_.literals[tree_.nodes[empty.literal_node].literal];
            if (literal.elements!=1 || literal.width!=1 || empty.source!="\"\"") error("empty literal operator string");
            result=empty.literal_node;
            tree_.nodes[result].kind=kind; tree_.nodes[result].payload=SyntaxPayload::none;
            tree_.nodes[result].operator_literal=true;
            tree_.append(result,name(SyntaxKind::Identifier));
        } else if (type_start()) {
            result=tree_.node(kind); tree_.nodes[result].operator_conversion=true;
            NodeId type=tree_.node(SyntaxKind::TypeId); tree_.append(type,specs(true));
            // conversion-type-id has ptr operators, not function/array suffixes.
            if (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND)) {
                NodeId d=tree_.node(SyntaxKind::AbstractDeclarator);
                while (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND)) {
                    tree_.append(d,leaf(SyntaxKind::Pointer,take()));
                    while (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE)) tree_.append(d,leaf(SyntaxKind::CvQualifier,take()));
                }
                tree_.append(type,d);
            }
            tree_.append(result,type);
        } else {
            PostToken op=take();
            if (op.kind!=PostKind::simple) error("operator token");
            result=leaf(kind,op); tree_.nodes[result].is_operator=true;
            if (op.simple==SimpleKind::OP_LPAREN) require(SimpleKind::OP_RPAREN);
            else if (op.simple==SimpleKind::OP_LSQUARE) require(SimpleKind::OP_RSQUARE);
            else if ((op.simple==SimpleKind::KW_NEW || op.simple==SimpleKind::KW_DELETE) && eat(SimpleKind::OP_LSQUARE)) {
                require(SimpleKind::OP_RSQUARE); tree_.nodes[result].operator_array=true;
            }
        }
    } else result=name(kind);
    if (global) tree_.nodes[result].global_scope=true;
    while (eat(SimpleKind::OP_COLON2)) {
        if (eat(SimpleKind::OP_STAR)) { tree_.nodes[result].kind=SyntaxKind::Pointer; tree_.nodes[result].member_pointer=true; break; }
        tree_.append(result,name(SyntaxKind::Identifier));
    }
    return result;
}
}
