#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <cassert>
#include <iostream>
#include <sstream>
using namespace cppgm;
int main(int argc, char** argv) {
    assert(argc==2);
    IdentifierTable ids;
    Preprocessor pp(ids,argv[1],"\"Oct 5 2026\"","\"00:00:00\"");
    PostCursor cursor(pp,ids,true); SyntaxTree tree; NodeId root;
    std::size_t tokens,queries,lookahead;
    { SyntaxParser parser(cursor,ids,tree); root=parser.parse(); assert(root);
      tokens=parser.tokens(); queries=parser.queries(); lookahead=parser.max_lookahead(); }
    std::vector<unsigned> seen(tree.nodes.size()); std::vector<NodeId> work(1,root);
    unsigned templates=0,arguments=0,parameters=0,factored=0,dependent=0,conversion=0;
    SyntaxScopeId owner=0;
    while (!work.empty()) {
        NodeId id=work.back(); work.pop_back(); assert(id && id<seen.size() && !seen[id]++);
        const auto& n=tree.nodes[id];
        assert(n.location.line && n.location.offset<=pp.source_buffer(n.location.file).bytes.size());
        assert(n.range.end>=n.range.begin);
        if (n.kind==SyntaxKind::Namespace) owner=n.scope;
        if (n.kind==SyntaxKind::TemplateDeclaration) { ++templates; assert(n.scope && n.first); }
        if (n.kind==SyntaxKind::TemplateArguments) ++arguments;
        if (n.kind==SyntaxKind::TypeParameter || n.kind==SyntaxKind::NonTypeParameter) ++parameters;
        if (n.kind==SyntaxKind::FactoredSyntax) ++factored;
        if (n.template_keyword || n.typename_keyword) ++dependent;
        if (n.operator_conversion) ++conversion;
        if (n.scope) assert(n.scope<tree.scopes.size());
        if (n.resolved_scope) assert(n.resolved_scope<tree.scopes.size());
        unsigned last=0;
        for (auto e=n.first;e;e=tree.edges.at(e).next) { work.push_back(tree.edges[e].child); last=e; }
        assert(last==n.last);
    }
    for (std::size_t i=1;i<seen.size();++i) assert(seen[i]==1);
    assert(templates && arguments && parameters && factored && dependent && conversion && owner);
    assert(lookahead<=4);
    auto key=[&](const char* s) { return ids.intern(s); };
    // Template publication must preserve categories; explicit instantiation must
    // not publish a value/type over the template, nor leak its parameter frame.
    auto& names=tree.scopes[owner].names;
    assert(!names.find(key("count")) && !names.find(key("item")) && !names.find(key("values")));
    auto function=names.at(key("identity"));
    assert(function.category==SyntaxCategory::templ && !function.template_type);
    auto type=names.at(key("number"));
    assert(type.category==SyntaxCategory::templ && type.template_type);
    auto nodes=tree.nodes.size(),edges=tree.edges.size();
    std::ostringstream a,b; tree.dump(a,ids,root); tree.dump(b,ids,root);
    assert(a.str()==b.str() && nodes==tree.nodes.size() && edges==tree.edges.size());
    std::cout << "PA5 retained template graph passed: nodes=" << nodes-1
              << " tokens=" << tokens << " queries=" << queries << " lookahead=" << lookahead << '\n';
}
