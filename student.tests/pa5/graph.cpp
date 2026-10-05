#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <cstring>
using namespace cppgm;
int main(int argc, char** argv) {
    assert(argc==2);
    IdentifierTable ids;
    Preprocessor pp(ids,argv[1],"\"Oct  5 2026\"","\"00:00:00\"");
    PostCursor cursor(pp,ids,true); SyntaxTree tree; SyntaxParser parser(cursor,ids,tree);
    NodeId root=parser.parse();
    std::vector<unsigned> seen(tree.nodes.size()); std::vector<NodeId> pending(1,root);
    unsigned literals=0, names=0, pointers=0, captures=0;
    while (!pending.empty()) {
        NodeId id=pending.back(); pending.pop_back(); assert(id && id<seen.size()); assert(!seen[id]++);
        const auto& n=tree.nodes[id];
        if (n.payload==SyntaxPayload::identifier) { assert(n.name); ++names; }
        if (n.payload==SyntaxPayload::literal) {
            const auto& v=tree.literals.at(n.literal);
            assert(v.kind!=PostKind::invalid); assert(n.location.line);
            assert(n.location.offset<=pp.source_buffer(n.location.file).bytes.size());
            assert(n.range.end>=n.range.begin);
            ++literals;
        }
        if (n.member_pointer) ++pointers;
        if (n.kind==SyntaxKind::Capture) ++captures;
        unsigned last=0;
        for (auto e=n.first;e;e=tree.edges.at(e).next) { pending.push_back(tree.edges[e].child); last=e; }
        assert(last==n.last);
    }
    for (std::size_t i=1;i<seen.size();++i) assert(seen[i]==1); // no abandoned speculative tree
    assert(literals && names && pointers && captures);
    assert(parser.max_lookahead()<=4);
    // Destruction/output walk is nonrecursive; semantic/literal identity survives dump.
    auto count=tree.nodes.size(); auto values=tree.values;
    std::ostringstream a,b; tree.dump(a,ids,root); tree.dump(b,ids,root);
    assert(a.str()==b.str()); assert(tree.nodes.size()==count); assert(tree.values==values);
    std::cout << "PA5 graph ownership/source/literal/name checks passed: nodes=" << count-1
              << " queries=" << parser.queries() << " lookahead=" << parser.max_lookahead() << '\n';
}
