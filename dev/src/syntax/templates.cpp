#include "syntax/parser.h"
namespace cppgm {
bool SyntaxParser::angle_end() {
    return !angle_boundaries_.empty() && angle_boundaries_.back()==delimiter_depth_ &&
        (at(SimpleKind::OP_GT) || at(SimpleKind::OP_RSHIFT));
}
bool SyntaxParser::close_angle() {
    if (at(SimpleKind::OP_RSHIFT)) {
        // Consume one logical half, leaving the second at the same source range.
        // Outside an angle boundary the original token remains a shift operator.
        peek(); lookahead_.front().simple=SimpleKind::OP_GT;
        InputToken second=lookahead_.front(); second.counted=true;
        take(); lookahead_.push_front(std::move(second)); return true;
    }
    return require(SimpleKind::OP_GT);
}
NodeId SyntaxParser::template_argument() {
    if (expected_) return 0;
    prepare_name();
    // A builtin-led conversion with an expression operand is not a function
    // type argument. Factor its bounded prefix using the same category facts
    // as declarator parameter clauses; never speculate or replay the region.
    if (peek().kind==PostKind::simple && builtin(peek().simple) && at(SimpleKind::OP_LPAREN,1) &&
        !at(SimpleKind::OP_RPAREN,2) && !type_start(2)) return expression(2);
    if (type_start()) return type_id();
    return expression(2);
}
NodeId SyntaxParser::template_arguments() {
    if (!require(SimpleKind::OP_LT)) return 0;
    angle_boundaries_.push_back(delimiter_depth_);
    NodeId result=tree_.node(SyntaxKind::TemplateArguments);
    if (!angle_end()) do {
        NodeId a=template_argument();
        if (eat(SimpleKind::OP_DOTS)) {
            NodeId pack=tree_.node(SyntaxKind::PackExpansion); tree_.append(pack,a); a=pack;
        }
        tree_.append(result,a);
    } while (!expected_ && eat(SimpleKind::OP_COMMA));
    close_angle(); angle_boundaries_.pop_back(); return result;
}
NodeId SyntaxParser::template_clause() {
    if (!require(SimpleKind::OP_LT)) return 0;
    angle_boundaries_.push_back(delimiter_depth_);
    NodeId result=tree_.node(SyntaxKind::TemplateClause);
    if (!angle_end()) {
        NodeId params=tree_.node(SyntaxKind::TemplateParameters); tree_.append(result,params);
        do {
            NodeId p=0; bool templ=at(SimpleKind::KW_TEMPLATE);
            if (templ || at(SimpleKind::KW_CLASS) || (at(SimpleKind::KW_TYPENAME) && peek(1).kind==PostKind::identifier && !at(SimpleKind::OP_COLON2,2) && !at(SimpleKind::OP_LT,2)) || (at(SimpleKind::KW_TYPENAME) && (at(SimpleKind::OP_COMMA,1) || at(SimpleKind::OP_GT,1) || at(SimpleKind::OP_DOTS,1) || at(SimpleKind::OP_ASS,1)))) {
                p=tree_.node(SyntaxKind::TypeParameter);
                if (templ) {
                    take(); tree_.append(p,tree_.node(SyntaxKind::TemplateTemplateParameter));
                    enter(); tree_.nodes[p].scope=active_.back(); tree_.append(p,template_clause()); leave();
                }
                if (!at(SimpleKind::KW_CLASS) && !at(SimpleKind::KW_TYPENAME)) return error("template parameter key");
                tree_.append(p,leaf(SyntaxKind::ParameterKey,take()));
                if (eat(SimpleKind::OP_DOTS)) tree_.append(p,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
                if (peek().kind==PostKind::identifier) {
                    NodeId id=name(SyntaxKind::Identifier); tree_.append(p,id);
                    bind(tree_.nodes[id].name,templ ? Category::templ : Category::type);
                }
                if (eat(SimpleKind::OP_ASS)) {
                    NodeId d=tree_.node(SyntaxKind::DefaultTemplateArgument); tree_.append(d,type_id(false,true)); tree_.append(p,d);
                }
            } else {
                p=tree_.node(SyntaxKind::NonTypeParameter); tree_.append(p,specs());
                NodeId first_spec=tree_.child(tree_.child(p));
                bool unnamed=at(SimpleKind::OP_ASS) && first_spec && tree_.nodes[first_spec].payload==SyntaxPayload::token;
                if (eat(SimpleKind::OP_DOTS)) tree_.append(p,raw(SyntaxKind::ParameterPack,SimpleKind::OP_DOTS));
                if (!at(SimpleKind::OP_COMMA) && !angle_end() && !at(SimpleKind::OP_ASS)) {
                    NodeId d=declarator(false,true); tree_.append(p,d); bind(declared_name(d),Category::value);
                }
                if (eat(SimpleKind::OP_ASS)) {
                    NodeId d=tree_.node(SyntaxKind::DefaultTemplateArgument); NodeId value=expression(2);
                    if (unnamed && value && tree_.nodes[value].payload==SyntaxPayload::literal) tree_.nodes[value].token_literal_view=true;
                    tree_.append(d,value); tree_.append(p,d);
                }
            }
            tree_.append(params,p);
        } while (!expected_ && eat(SimpleKind::OP_COMMA));
    }
    close_angle(); angle_boundaries_.pop_back(); return result;
}
NodeId SyntaxParser::template_declaration() {
    bool ext=eat(SimpleKind::KW_EXTERN); if (!require(SimpleKind::KW_TEMPLATE)) return 0;
    if (!at(SimpleKind::OP_LT)) {
        NodeId result=tree_.node(SyntaxKind::ExplicitInstantiation);
        if (ext) tree_.nodes[result].kind=SyntaxKind::ExplicitInstantiationDeclaration;
        // An instantiated function declarator cannot have a function body.
        ++instantiation_depth_; NodeId body=declaration(); --instantiation_depth_;
        if (body && (tree_.nodes[body].kind==SyntaxKind::FunctionDefinition || tree_.nodes[body].kind==SyntaxKind::Class)) return error("explicit instantiation without a body");
        tree_.append(result,body); return result;
    }
    if (ext) return error("template instantiation declaration");
    NodeId result=tree_.node(SyntaxKind::TemplateDeclaration);
    SyntaxScopeId owner=active_.back(); enter(); tree_.nodes[result].scope=active_.back();
    tree_.append(result,template_clause());
    ++template_depth_; NodeId body=declaration(); --template_depth_; tree_.append(result,body);
    // Publish only the declared entity, never the parameter environment. The
    // class/function graph retains that environment as its indexed parent.
    NodeId entity=body;
    if (body && (tree_.nodes[body].kind==SyntaxKind::SimpleDeclaration || (tree_.nodes[body].kind==SyntaxKind::FunctionDefinition || tree_.nodes[body].kind==SyntaxKind::Class))) {
        for (auto e=tree_.nodes[body].first;e;e=tree_.edges[e].next) {
            NodeId c=tree_.edges[e].child;
            if (tree_.nodes[c].kind==SyntaxKind::Declarator) { entity=c; break; }
            if (tree_.nodes[c].kind==SyntaxKind::InitDeclarators) { entity=tree_.child(tree_.child(c)); break; }
        }
    }
    IdentifierId id=body && (tree_.nodes[body].kind==SyntaxKind::Class || tree_.nodes[body].kind==SyntaxKind::ClassForward || tree_.nodes[body].kind==SyntaxKind::Alias) ? tree_.nodes[body].name : declared_name(entity);
    SyntaxScopeId target=body ? tree_.nodes[body].scope : 0;
    leave(); if (active_.back()!=owner) return error("template scope");
    bind(id,Category::templ,target);
    if (id) tree_.scopes[owner].names[id].template_type=body && (tree_.nodes[body].kind==SyntaxKind::Class || tree_.nodes[body].kind==SyntaxKind::ClassForward || tree_.nodes[body].kind==SyntaxKind::Alias);
    return result;
}
}
