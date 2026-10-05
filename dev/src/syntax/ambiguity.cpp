#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::ambiguous_statement() {
    // Factor type(expr/declarator) once. No speculative token retention, replay,
    // category mutation or abandoned tree. Reclassify the minimal common nodes.
    const PostToken type=peek();
    NodeId spec=specs(); require(SimpleKind::OP_LPAREN);
    NodeId inner=0;
    bool declaration_candidate=false;
    prepare_name();
    if ((peek().kind==PostKind::identifier && lookahead_.front().name_node && tree_.nodes[lookahead_.front().name_node].member_pointer) || at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) ||
        (peek().kind==PostKind::identifier && at(SimpleKind::OP_RPAREN,1))) {
        inner=declarator(); declaration_candidate=true;
    } else if (!at(SimpleKind::OP_RPAREN)) {
        inner=expression(2);
        while (eat(SimpleKind::OP_COMMA)) {
            NodeId args=tree_.node(SyntaxKind::Arguments); tree_.append(args,inner);
            tree_.append(args,expression(2));
            while (eat(SimpleKind::OP_COMMA)) tree_.append(args,expression(2));
            inner=args;
        }
    }
    require(SimpleKind::OP_RPAREN);
    const bool declaration_tail=at(SimpleKind::OP_SEMICOLON) || at(SimpleKind::OP_COMMA) || at(SimpleKind::OP_ASS) ||
        at(SimpleKind::OP_LBRACE) || at(SimpleKind::OP_LSQUARE) || at(SimpleKind::OP_LPAREN);
    if (declaration_candidate && declaration_tail) {
        NodeId result=tree_.node(SyntaxKind::SimpleDeclaration); tree_.append(result,spec);
        NodeId d=tree_.node(SyntaxKind::Declarator), nested=tree_.node(SyntaxKind::NestedDeclarator);
        tree_.append(nested,inner); tree_.append(d,nested); suffixes(d);
        NodeId items=tree_.node(SyntaxKind::InitDeclarators);
        while (true) {
            bind(declared_name(d),Category::value);
            NodeId item=tree_.node(SyntaxKind::InitDeclarator); tree_.append(item,d);
            if (eat(SimpleKind::OP_ASS)) tree_.append(item,initializer(true));
            else if (at(SimpleKind::OP_LPAREN)) tree_.append(item,initializer());
            else if (at(SimpleKind::OP_LBRACE)) { NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(item,i); }
            tree_.append(items,item);
            if (!eat(SimpleKind::OP_COMMA)) break;
            d=declarator();
        }
        require(SimpleKind::OP_SEMICOLON); tree_.append(result,items); return result;
    }
    // Reuse the common type specifier/list nodes for the construction expression.
    NodeId callee=tree_.child(spec); tree_.nodes[callee].kind=SyntaxKind::IdExpression;
    tree_.nodes[callee].payload=type.kind==PostKind::identifier ? SyntaxPayload::identifier : SyntaxPayload::raw_token;
    if (!tree_.nodes[callee].first) tree_.nodes[callee].name=type.identifier;
    tree_.nodes[callee].token=type.simple;
    tree_.nodes[spec].kind=SyntaxKind::Call;
    NodeId args=0;
    if (declaration_candidate) {
        NodeId c=tree_.child(inner);
        if (!c || tree_.nodes[c].kind!=SyntaxKind::Identifier || tree_.edges[tree_.nodes[inner].first].next) error("expression argument");
        tree_.nodes[inner].kind=type.kind==PostKind::identifier ? SyntaxKind::Arguments : SyntaxKind::ParenArguments;
        tree_.nodes[c].kind=SyntaxKind::IdExpression;
        args=inner;
    } else if (inner && tree_.nodes[inner].kind==SyntaxKind::Arguments) {
        args=inner; tree_.nodes[args].kind=type.kind==PostKind::identifier ? SyntaxKind::Arguments : SyntaxKind::ParenArguments;
    } else { args=tree_.node(type.kind==PostKind::identifier ? SyntaxKind::Arguments : SyntaxKind::ParenArguments); if (inner) tree_.append(args,inner); }
    tree_.append(spec,args);
    NodeId result=tree_.node(SyntaxKind::ExpressionStatement); tree_.append(result,expression_tail(postfix(spec)));
    require(SimpleKind::OP_SEMICOLON); return result;
}
}
