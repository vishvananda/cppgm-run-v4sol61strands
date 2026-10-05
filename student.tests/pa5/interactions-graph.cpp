#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <cassert>
#include <sstream>
#include <iostream>
using namespace cppgm;
int main(int argc, char** argv) {
    assert(argc==2);
    IdentifierTable ids;
    Preprocessor pp(ids,argv[1],"\"Oct  5 2026\"","\"00:00:00\"");
    PostCursor cursor(pp,ids,true); SyntaxTree tree; NodeId root;
    { SyntaxParser parser(cursor,ids,tree); root=parser.parse(); assert(root && parser.max_lookahead()<=4); }
    std::vector<unsigned> seen(tree.nodes.size()); std::vector<NodeId> work(1,root);
    while (!work.empty()) {
        auto id=work.back(); work.pop_back(); assert(id && id<seen.size() && !seen[id]++);
        const auto& n=tree.nodes[id];
        assert((id==root || n.location.line) && n.location.offset<=pp.source_buffer(n.location.file).bytes.size());
        assert(n.range.begin<=n.range.end);
        unsigned last=0;
        for (auto e=n.first;e;e=tree.edges.at(e).next) { work.push_back(tree.edges[e].child); last=e; }
        assert(last==n.last);
    }
    for (std::size_t i=1;i<seen.size();++i) assert(seen[i]==1);
    for (std::size_t i=1;i<tree.scopes.size();++i) {
        const auto& s=tree.scopes[i]; assert(s.parent<tree.scopes.size());
        if (s.names_owner) {
            assert(s.names_owner<tree.scopes.size());
            assert(tree.scopes[s.names_owner].template_environment);
            assert(s.names.begin()==s.names.end()); // overlay never copies bindings
        }
        unsigned depth=0;
        for (auto parent=s.parent;parent;parent=tree.scopes[parent].parent) assert(++depth<tree.scopes.size());
    }
    auto nodes=tree.nodes.size(),edges=tree.edges.size(),scopes=tree.scopes.size();
    std::ostringstream a,b; tree.dump(a,ids,root); tree.dump(b,ids,root);
    assert(a.str()==b.str() && nodes==tree.nodes.size() && edges==tree.edges.size() && scopes==tree.scopes.size());
    std::cout << "PA5 final interaction graph passed: " << nodes-1 << " nodes, " << scopes-1 << " scopes\n";
}
