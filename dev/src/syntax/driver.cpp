#include "syntax/parser.h"
#include "preprocess/engine/preprocessor.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <stdexcept>
namespace cppgm {
int emit_ast(const std::string& output, const std::vector<std::string>& inputs, bool telemetry) {
    std::ofstream out(output);
    if (!out) throw std::runtime_error("cannot open AST output");
    out << inputs.size() << " translation units\n";
    std::time_t now=std::time(nullptr); auto tm=*std::localtime(&now); char date[32], time[32];
    std::strftime(date,sizeof date,"\"%b %e %Y\"",&tm); std::strftime(time,sizeof time,"\"%H:%M:%S\"",&tm);
    for (std::size_t i=0;i<inputs.size();++i) {
        auto start=std::chrono::steady_clock::now();
        IdentifierTable ids; Preprocessor pp(ids,inputs[i],date,time,telemetry);
        PostCursor cursor(pp,ids,true); SyntaxTree tree; SyntaxParser parser(cursor,ids,tree);
        NodeId root=parser.parse(); auto parsed=std::chrono::steady_clock::now();
        out << "start translation unit " << i+1 << '\n'; tree.dump(out,ids,root); out << "end translation unit\n";
        if (telemetry) {
            auto done=std::chrono::steady_clock::now();
            std::cerr << "syntax tokens=" << parser.tokens() << " nodes=" << tree.nodes.size()-1
                << " edges=" << tree.edges.size()-1 << " category_queries=" << parser.queries()
                << " max_lookahead=" << parser.max_lookahead()
                << " parse_seconds=" << std::chrono::duration<double>(parsed-start).count()
                << " dump_seconds=" << std::chrono::duration<double>(done-parsed).count() << '\n';
        }
    }
    if (!out) throw std::runtime_error("cannot write AST output");
    return EXIT_SUCCESS;
}
}
