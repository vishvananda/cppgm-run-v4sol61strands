#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::parameter_expression(NodeId p) {
    NodeId spec=tree_.child(p); NodeId d=tree_.nodes[p].first ? tree_.edges[tree_.nodes[p].first].next : 0;
    d=d ? tree_.edges[d].child : 0;
    NodeId callee=tree_.child(spec);
    if (!callee) return error("initializer expression type");
    auto& c=tree_.nodes[callee]; c.kind=SyntaxKind::IdExpression;
    bool builtin_type=c.payload==SyntaxPayload::token;
    if (builtin_type) c.payload=SyntaxPayload::raw_token;
    tree_.nodes[p].kind=SyntaxKind::Call;
    // Keep the factored wrappers owned by the graph. They are transparent in
    // explicit views, but no parsed node or source range is abandoned.
    tree_.nodes[spec].kind=SyntaxKind::FactoredSyntax;
    NodeId args=0;
    if (d) {
        tree_.nodes[d].kind=SyntaxKind::FactoredSyntax;
        args=tree_.child(d);
        if (args && tree_.nodes[args].kind==SyntaxKind::Parameters) {
            tree_.nodes[args].kind=builtin_type ? SyntaxKind::ParenArguments : SyntaxKind::Arguments;
            for (auto e=tree_.nodes[args].first;e;e=tree_.edges[e].next) tree_.edges[e].child=parameter_expression(tree_.edges[e].child);
        } else return error("initializer expression argument");
    }
    if (!args) tree_.append(p,tree_.node(builtin_type ? SyntaxKind::ParenArguments : SyntaxKind::Arguments));
    return p;
}
NodeId SyntaxParser::ambiguous_statement(NodeId spec, bool builtin_type) {
    if (expected_) return 0;
    // Factor type(expr/declarator) once. No speculative token retention, replay,
    // category mutation or abandoned tree. Reclassify the minimal common nodes.
    if (!spec) {
        builtin_type=peek().kind==PostKind::simple && builtin(peek().simple);
        spec=specs();
    }
    if (!require(SimpleKind::OP_LPAREN)) return 0;
    NodeId inner=0;
    bool declaration_candidate=false;
    prepare_name();
    if ((peek().kind==PostKind::identifier && lookahead_.front().name_node && tree_.nodes[lookahead_.front().name_node].member_pointer) || at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND) ||
        (peek().kind==PostKind::identifier && at(SimpleKind::OP_RPAREN,1))) {
        inner=declarator(); declaration_candidate=true;
    } else if (!at(SimpleKind::OP_RPAREN)) {
        inner=expression(2);
        while (!expected_ && (eat(SimpleKind::OP_COMMA))) {
            NodeId args=tree_.node(SyntaxKind::Arguments); tree_.append(args,inner);
            tree_.append(args,expression(2));
            while (!expected_ && (eat(SimpleKind::OP_COMMA))) tree_.append(args,expression(2));
            inner=args;
        }
    }
    if (!require(SimpleKind::OP_RPAREN)) { return 0; }
    const bool declaration_tail=at(SimpleKind::OP_SEMICOLON) || at(SimpleKind::OP_COMMA) || at(SimpleKind::OP_ASS) ||
        at(SimpleKind::OP_LBRACE) || at(SimpleKind::OP_LSQUARE) || at(SimpleKind::OP_LPAREN);
    if (declaration_candidate && declaration_tail) {
        NodeId result=tree_.node(SyntaxKind::SimpleDeclaration); tree_.append(result,spec);
        NodeId d=tree_.node(SyntaxKind::Declarator), nested=tree_.node(SyntaxKind::NestedDeclarator);
        tree_.append(nested,inner); tree_.append(d,nested); suffixes(d);
        NodeId items=tree_.node(SyntaxKind::InitDeclarators);
        while (!expected_ && (true)) {
            bind(declared_name(d),Category::value);
            NodeId item=tree_.node(SyntaxKind::InitDeclarator); tree_.append(item,d);
            if (eat(SimpleKind::OP_ASS)) tree_.append(item,initializer(true));
            else if (at(SimpleKind::OP_LPAREN)) tree_.append(item,initializer());
            else if (at(SimpleKind::OP_LBRACE)) { NodeId i=tree_.node(SyntaxKind::Initializer); tree_.append(i,initializer()); tree_.append(item,i); }
            tree_.append(items,item);
            if (!eat(SimpleKind::OP_COMMA)) break;
            d=declarator();
        }
        if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } tree_.append(result,items); return result;
    }
    // Reuse the common type specifier/list nodes for the construction expression.
    NodeId callee=tree_.child(spec); tree_.nodes[callee].kind=SyntaxKind::IdExpression;
    if (builtin_type) tree_.nodes[callee].payload=SyntaxPayload::raw_token;
    tree_.nodes[spec].kind=SyntaxKind::Call;
    NodeId args=0;
    if (declaration_candidate) {
        NodeId c=tree_.child(inner);
        if (!c || tree_.nodes[c].kind!=SyntaxKind::Identifier || tree_.edges[tree_.nodes[inner].first].next) return error("expression argument");
        tree_.nodes[inner].kind=builtin_type ? SyntaxKind::ParenArguments : SyntaxKind::Arguments;
        tree_.nodes[c].kind=SyntaxKind::IdExpression;
        args=inner;
    } else if (inner && tree_.nodes[inner].kind==SyntaxKind::Arguments) {
        args=inner; tree_.nodes[args].kind=builtin_type ? SyntaxKind::ParenArguments : SyntaxKind::Arguments;
    } else { args=tree_.node(builtin_type ? SyntaxKind::ParenArguments : SyntaxKind::Arguments); if (inner) tree_.append(args,inner); }
    tree_.append(spec,args);
    NodeId result=tree_.node(SyntaxKind::ExpressionStatement); tree_.append(result,expression_tail(postfix(spec)));
    if (!require(SimpleKind::OP_SEMICOLON)) { return 0; } return result;
}
}
