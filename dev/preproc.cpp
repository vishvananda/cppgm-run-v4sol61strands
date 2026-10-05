// (C) 2013 CPPGM Foundation www.cppgm.org. All rights reserved.
#include "preprocess/engine/preprocessor.h"
#include "preprocess/post/cursor.h"
#include "preprocess/post/render.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include <chrono>

int main(int argc,char** argv) {
    try {
        if(argc<4 || std::string(argv[1])!="-o") throw std::runtime_error("usage: preproc -o output source...");
        std::time_t now=std::time(nullptr); std::string clock=std::asctime(std::localtime(&now));
        std::string date=clock.substr(4,7)+clock.substr(20,4), time=clock.substr(11,8);
        std::ofstream out(argv[2]); if(!out) throw std::runtime_error("cannot open output");
        out<<"preproc "<<argc-3<<'\n';
        for(int i=3;i<argc;++i) {
            auto start=std::chrono::steady_clock::now();
            cppgm::IdentifierTable ids; cppgm::Preprocessor preprocessor(ids,argv[i],date,time,std::getenv("CPPGM_METRICS")!=nullptr);
            cppgm::PostCursor tokens(preprocessor,ids,true);
            out<<"sof "<<argv[i]<<'\n';
            for (;;) {
                auto token=tokens.next();
                if(token.kind==cppgm::PostKind::eof) break;
                if(token.kind==cppgm::PostKind::invalid) throw std::runtime_error("invalid phase-7 token");
                cppgm::render_posttoken(out,token,ids);
            }
            out<<"eof\n";
            if(std::getenv("CPPGM_METRICS")) {
                const auto& m=preprocessor.metrics();
                std::cerr<<"preproc bytes="<<m.source_bytes<<" source_tokens="<<m.source_tokens<<" expanded="<<m.expanded<<" invocations="<<m.invocations<<" argument_tokens="<<m.argument_tokens<<" pastes="<<m.paste_tokens<<" lookups="<<m.lookups<<" max_pending="<<m.max_pending<<" paint_nodes="<<m.paint_nodes<<" paint_queries="<<m.paint_queries<<" paint_cache_hits="<<m.paint_cache_hits<<" indexed_tokens="<<m.indexed_tokens<<" argument_spans="<<m.argument_spans<<" max_tasks="<<m.max_tasks<<" files="<<m.files<<" conditions="<<m.conditions<<" expression_nodes="<<m.expression_nodes<<" expression_reductions="<<m.expression_reductions<<" expression_max_nodes="<<m.expression_max_nodes<<" expression_max_stack="<<m.expression_max_stack<<" expression_parse_seconds="<<m.expression_parse_seconds<<" expression_evaluate_seconds="<<m.expression_evaluate_seconds<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
            }
        }
        if(!out) throw std::runtime_error("output write failed");
        return EXIT_SUCCESS;
    } catch(const std::exception& e) {std::cerr<<"ERROR: "<<e.what()<<'\n';return EXIT_FAILURE;}
}
