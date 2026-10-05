#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace cppgm;
int main(int argc,char** argv) {
    assert(argc==2);
    IdentifierTable ids;
    Preprocessor pp(ids,argv[1],"\"Oct 5 2026\"","\"00:00:00\"");
    PostCursor cursor(pp,ids,true); SyntaxTree tree; NodeId root;
    { SyntaxParser parser(cursor,ids,tree); root=parser.parse(); }
    std::vector<unsigned> seen(tree.nodes.size()); std::vector<NodeId> work(1,root);
    unsigned resolved=0, namespaces=0, enums=0, qualifiers=0;
    while (!work.empty()) {
        auto id=work.back(); work.pop_back(); assert(id && id<seen.size() && !seen[id]++);
        const auto& n=tree.nodes[id];
        if (n.scope) { assert(n.scope<tree.scopes.size()); assert(n.location.line); }
        if (n.resolved_scope) { assert(n.resolved_scope<tree.scopes.size()); ++resolved; }
        if (n.kind==SyntaxKind::Namespace) { assert(n.scope); ++namespaces; }
        if (n.kind==SyntaxKind::Enum) { assert(n.scope); ++enums; }
        if (n.terminal_name) { assert(n.category!=SyntaxCategory::unknown); ++qualifiers; }
        for (auto e=n.first;e;e=tree.edges[e].next) work.push_back(tree.edges[e].child);
    }
    for (std::size_t i=1;i<seen.size();++i) assert(seen[i]==1);
    for (std::size_t i=1;i<tree.scopes.size();++i) {
        const auto& s=tree.scopes[i]; assert(s.parent<i);
        for (auto edge:s.imports) assert(edge && edge<tree.scopes.size());
        for (const auto& b:s.names) {
            assert(b.second.target<tree.scopes.size()); assert(b.second.qualifier<tree.scopes.size());
        }
    }
    assert(resolved && namespaces && enums && qualifiers);
    std::ostringstream a,b; tree.dump(a,ids,root); tree.dump(b,ids,root); assert(a.str()==b.str());
    std::cout << "retained scope/name graph: " << tree.nodes.size()-1 << " nodes, " << tree.scopes.size()-1 << " scopes\n";
}
