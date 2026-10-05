#include "syntax/parser.h"
namespace cppgm {
NodeId SyntaxParser::qualified_component(SyntaxKind kind) {
    if (expected_) return 0;
    // Every name component has compact identity. The joined leaf is dump-only.
    NodeId result=0;
    if (eat(SimpleKind::KW_OPERATOR)) {
        if (peek().kind==PostKind::array) {
            auto empty=take();
            const auto& literal=tree_.literals[tree_.nodes[empty.literal_node].literal];
            if (literal.elements!=1 || literal.width!=1) return error("empty literal operator string");
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
                while (!expected_ && (at(SimpleKind::OP_STAR) || at(SimpleKind::OP_AMP) || at(SimpleKind::OP_LAND))) {
                    tree_.append(d,leaf(SyntaxKind::Pointer,take()));
                    while (!expected_ && (at(SimpleKind::KW_CONST) || at(SimpleKind::KW_VOLATILE))) tree_.append(d,leaf(SyntaxKind::CvQualifier,take()));
                }
                tree_.append(type,d);
            }
            tree_.append(result,type);
        } else {
            PostToken op=take();
            if (op.kind!=PostKind::simple) return error("operator token");
            result=leaf(kind,op); tree_.nodes[result].is_operator=true;
            if (op.simple==SimpleKind::OP_LPAREN) { if (!require(SimpleKind::OP_RPAREN)) { return 0; } }
            else if (op.simple==SimpleKind::OP_LSQUARE) { if (!require(SimpleKind::OP_RSQUARE)) { return 0; } }
            else if ((op.simple==SimpleKind::KW_NEW || op.simple==SimpleKind::KW_DELETE) && eat(SimpleKind::OP_LSQUARE)) {
                if (!require(SimpleKind::OP_RSQUARE)) { return 0; } tree_.nodes[result].operator_array=true;
            }
        }
    } else if (eat(SimpleKind::OP_COMPL)) {
        result=name(kind); tree_.nodes[result].destructor=true;
    } else result=name(kind);
    return result;
}
NodeId SyntaxParser::qualified(SyntaxKind kind, bool namespace_only) {
    if (expected_) return 0;
    if (peek().kind==PostKind::identifier && lookahead_.front().name_node) {
        NodeId n=take().name_node; tree_.nodes[n].kind=kind; return n;
    }
    return qualified_raw(kind,namespace_only);
}
NodeId SyntaxParser::qualified_raw(SyntaxKind kind, bool namespace_only) {
    if (expected_) return 0;
    bool global=eat(SimpleKind::OP_COLON2);
    SyntaxScopeId scope=global ? active_.front() : active_.back();
    NodeId result=qualified_component(kind), component=result;
    tree_.nodes[result].global_scope=global;
    bool parents=!global;
    bool member=member_name_; member_name_=false;
    while (!expected_ && (true)) {
        auto id=tree_.nodes[component].name;
        auto binding=(id && !(member && !at(SimpleKind::OP_COLON2))) ? lookup(id,scope,parents,at(SimpleKind::OP_COLON2),namespace_only) : SyntaxBinding{};
        Category fact=binding.category==Category::unknown ? (id ? hint(id) : Category::value) : binding.category;
        if (member && !at(SimpleKind::OP_COLON2)) fact=Category::value;
        if (at(SimpleKind::OP_LT) && (fact==Category::templ || (binding.category==Category::unknown && !member  && type_start(1)) || tree_.nodes[component].is_operator || tree_.nodes[component].operator_literal || tree_.nodes[component].template_keyword))
            tree_.append(component,template_arguments());
        bool qualifier=at(SimpleKind::OP_COLON2);
        if (qualifier && id) binding=lookup(id,scope,parents,true);
        auto& node=tree_.nodes[result];
        node.terminal_name=id;
        node.category=binding.category==Category::unknown ? (id ? hint(id) : Category::value) : binding.category;
        if (binding.category==Category::templ && !binding.template_type) node.category=Category::value;
        node.resolved_scope=binding.target;
        if (!qualifier) break;
        take(); scope=binding.target; parents=false;
        if (eat(SimpleKind::OP_STAR)) { node.kind=SyntaxKind::Pointer; node.member_pointer=true; break; }
        tree_.nodes[result].qualifier_scope=scope;
        // A qualified declarator's conversion type is looked up in its class.
        // Temporarily nominate that indexed environment, not the rendered prefix.
        if (scope) enter(scope);
        member=false;
        bool explicit_template=eat(SimpleKind::KW_TEMPLATE);
        component=qualified_component(SyntaxKind::Identifier);
        tree_.nodes[component].template_keyword=explicit_template;
        if (scope) leave();
        if (tree_.nodes[component].operator_conversion) tree_.nodes[component].global_scope=true;
        tree_.append(result,component);
    }
    return result;
}
}
