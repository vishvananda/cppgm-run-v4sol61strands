#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
using namespace cppgm;
int main(int argc,char** argv) {
    assert(argc==2);
    IdentifierTable ids;
    Preprocessor pp(ids,argv[1],"\"Oct  5 2026\"","\"00:00:00\"");
    PostCursor cursor(pp,ids,true); SyntaxTree tree; NodeId root;
    std::size_t tokens,queries,lookahead;
    {
        SyntaxParser parser(cursor,ids,tree); root=parser.parse();
        tokens=parser.tokens(); queries=parser.queries(); lookahead=parser.max_lookahead();
    }
    std::vector<unsigned> seen(tree.nodes.size()); std::vector<NodeId> work(1,root);
    unsigned classes=0,bases=0,specials=0,defaults=0,initializers=0;
    while (!work.empty()) {
        NodeId id=work.back();work.pop_back();assert(id && id<seen.size());assert(!seen[id]++);
        const auto& n=tree.nodes[id];
        assert(n.location.line);
        assert(n.location.offset<=pp.source_buffer(n.location.file).bytes.size());
        assert(n.range.end>=n.range.begin);
        if (n.kind==SyntaxKind::Class) {
            ++classes; assert(n.scope && n.complete_definition);
            if (n.name) {
                auto b=tree.scopes[n.scope].names.at(n.name);
                assert(b.qualifier==n.scope && b.qualifier_category==SyntaxCategory::type);
            }
        }
        if (n.kind==SyntaxKind::Base) ++bases;
        if (n.kind==SyntaxKind::SpecialDefinition) ++specials;
        if (n.kind==SyntaxKind::DefaultArgument) { ++defaults; assert(n.first); }
        if (n.kind==SyntaxKind::Initializer) { ++initializers; assert(n.first); }
        if (n.literal) {
            const auto& lit=tree.literals.at(n.literal);
            if (lit.kind==PostKind::array) {
                assert(lit.length==lit.width*lit.elements);
                assert(lit.offset+lit.length<=tree.values.size());
            }
        }
        if (n.qualifier_scope) assert(n.qualifier_scope<tree.scopes.size());
        unsigned last=0;
        for (auto e=n.first;e;e=tree.edges.at(e).next) {work.push_back(tree.edges[e].child);last=e;}
        assert(last==n.last);
    }
    for (std::size_t i=1;i<seen.size();++i) assert(seen[i]==1);
    assert(classes && bases && specials && defaults && initializers);
    for (std::size_t s=1;s<tree.scopes.size();++s) {
        auto p=tree.scopes[s].parent; assert(p<tree.scopes.size());
        unsigned depth=0;
        while(p) {assert(++depth<tree.scopes.size());p=tree.scopes[p].parent;}
        for (auto b:tree.scopes[s].bases) assert(b && b<tree.scopes.size());
    }
    auto nodes=tree.nodes.size(),edges=tree.edges.size();auto values=tree.values;
    std::ostringstream a,b;tree.dump(a,ids,root);tree.dump(b,ids,root);
    assert(a.str()==b.str() && nodes==tree.nodes.size() && edges==tree.edges.size() && values==tree.values);
    assert(lookahead<=4);
    std::cout<<"PA5 retained class graph passed: nodes="<<nodes-1<<" tokens="<<tokens<<" queries="<<queries<<" lookahead="<<lookahead<<'\n';
}
