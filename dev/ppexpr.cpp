// (C) 2013 CPPGM Foundation www.cppgm.org. All rights reserved.
// Cumulative streaming controlling-expression frontend.
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sys/resource.h>
#include "preprocess/expr/expression.h"

namespace {
bool mock_defined(void* context, cppgm::IdentifierId id) {
    const auto spelling = static_cast<cppgm::IdentifierTable*>(context)->spelling(id);
    return spelling.size && (static_cast<unsigned char>(spelling.data[0]) & 1);
}
void render(cppgm::PPValue value) {
    if (value.is_unsigned) std::cout << value.bits << "u\n";
    else if (value.bits >> 63) std::cout << '-' << -value.bits << '\n';
    else std::cout << value.bits << '\n';
}
}
int main(int, char**) {
    try {
        std::ios::sync_with_stdio(false);
        const auto start = std::chrono::steady_clock::now();
        cppgm::SourceBuffer source{std::string(std::istreambuf_iterator<char>(std::cin),
                                             std::istreambuf_iterator<char>())};
        const auto loaded = std::chrono::steady_clock::now();
        cppgm::IdentifierTable identifiers;
        cppgm::LexerOptions options;
        options.collect_literal_elements = true; options.convert_empty_character = true;
        cppgm::Lexer lexer(source, identifiers, options);
        cppgm::PostCursor cursor(lexer, identifiers, false, true);
        const bool telemetry = std::getenv("CPPGM_EXPR_METRICS") != nullptr;
        cppgm::ControllingExpression expressions(cursor, identifiers, mock_defined, &identifiers, telemetry);
        cppgm::PPValue value; bool valid;
        while (expressions.next(value, valid)) {
            if (valid) render(value); else std::cout << "error\n";
        }
        std::cout << "eof\n";
        if (telemetry) {
            const auto end = std::chrono::steady_clock::now();
            struct rusage usage; getrusage(RUSAGE_SELF, &usage);
            const auto& e = expressions.metrics(); const auto& l = lexer.metrics();
            std::cerr << "expr bytes=" << l.physical_bytes << " pp_tokens=" << l.tokens
                      << " lines=" << e.lines << " nodes=" << e.nodes << " evaluated=" << e.evaluated
                      << " max_nodes=" << e.max_nodes << " max_stack=" << e.max_stack
                      << " reductions=" << e.reductions << " deferred_errors=" << e.deferred_errors
                      << " identifiers=" << identifiers.size() << " intern_probes=" << identifiers.probes()
                      << " read_us=" << std::chrono::duration_cast<std::chrono::microseconds>(loaded-start).count()
                      << " frontend_render_us=" << std::chrono::duration_cast<std::chrono::microseconds>(end-loaded).count()
                      << " parse_us=" << e.parse_seconds*1e6 << " evaluate_us=" << e.evaluate_seconds*1e6
                      << " peak_rss_kb=" << usage.ru_maxrss << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n'; return EXIT_FAILURE;
    }
}
