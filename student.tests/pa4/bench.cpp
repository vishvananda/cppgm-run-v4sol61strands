#include "preprocess/engine/preprocessor.h"
#include "preprocess/post/cursor.h"
#include <iostream>
#include <cstdlib>
int main(int argc,char** argv) {
    try {
        if(argc!=2) return 2;
        cppgm::IdentifierTable ids;
        cppgm::Preprocessor pp(ids,argv[1],"Oct  5 2026","00:00:00");
        cppgm::PostCursor cursor(pp,ids);
        std::size_t count=0;
        for (;;) {
            auto t=cursor.next(); if(t.kind==cppgm::PostKind::eof) break;
            if(t.kind!=cppgm::PostKind::identifier || !ids.spelling(t.identifier).equals("done")) return 3;
            ++count;
        }
        std::cout<<count<<'\n';
        const auto& m=pp.metrics();
        std::cerr<<"bytes="<<m.source_bytes<<" source_tokens="<<m.source_tokens<<" expanded="<<m.expanded<<" invocations="<<m.invocations<<" argument_tokens="<<m.argument_tokens<<" pastes="<<m.paste_tokens<<" max_pending="<<m.max_pending<<" lookups="<<m.lookups<<" paint_nodes="<<m.paint_nodes<<" paint_queries="<<m.paint_queries<<" paint_cache_hits="<<m.paint_cache_hits<<" indexed_tokens="<<m.indexed_tokens<<" argument_spans="<<m.argument_spans<<" max_tasks="<<m.max_tasks<<" files="<<m.files<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
