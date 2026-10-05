#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::qualified(SyntaxKind kind) {
    // Every name component has compact identity. The joined leaf is dump-only.
    bool global=eat(SimpleKind::OP_COLON2);
    NodeId result=0;
    if (eat(SimpleKind::KW_OPERATOR)) {
        PostToken op=take();
        if (op.kind!=PostKind::simple) error("operator token");
        result=tree_.token(kind,op.simple,op.location); tree_.nodes[result].is_operator=true;
        if (op.simple==SimpleKind::OP_LPAREN) require(SimpleKind::OP_RPAREN);
        else if (op.simple==SimpleKind::OP_LSQUARE) require(SimpleKind::OP_RSQUARE);
        else if ((op.simple==SimpleKind::KW_NEW || op.simple==SimpleKind::KW_DELETE) && eat(SimpleKind::OP_LSQUARE)) {
            require(SimpleKind::OP_RSQUARE); tree_.nodes[result].operator_array=true;
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
