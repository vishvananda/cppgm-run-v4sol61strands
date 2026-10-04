#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/resource.h>

#include "preprocess/lex/emit.h"
#include "preprocess/tokens/DebugPPTokenStream.h"

int main(int, char**) {
    try {
        std::ios::sync_with_stdio(false);
        const auto begin = std::chrono::steady_clock::now();
        cppgm::SourceBuffer source{std::string(std::istreambuf_iterator<char>(std::cin),
                                             std::istreambuf_iterator<char>())};
        const auto loaded = std::chrono::steady_clock::now();
        cppgm::IdentifierTable identifiers;
        cppgm::Lexer lexer(source, identifiers);
        DebugPPTokenStream output;
        for (;;) {
            cppgm::Token token = lexer.next();
            cppgm::emit_token(token, lexer.spelling(), output);
            if (token.kind == cppgm::TokenKind::eof) break;
        }
        if (std::getenv("CPPGM_LEX_METRICS")) {
            const auto end = std::chrono::steady_clock::now();
            struct rusage usage;
            getrusage(RUSAGE_SELF, &usage);
            const auto& m = lexer.metrics();
            std::cerr << "lexer bytes=" << m.physical_bytes << " decoded=" << m.decoded_characters
                      << " tokens=" << m.tokens << " spelling_bytes=" << m.spelling_bytes
                      << " identifiers=" << identifiers.size() << " intern_probes=" << identifiers.probes()
                      << " spelling_slabs=" << identifiers.slabs() << " raw_candidates=" << m.raw_candidates
                      << " read_us=" << std::chrono::duration_cast<std::chrono::microseconds>(loaded-begin).count()
                      << " lex_render_us=" << std::chrono::duration_cast<std::chrono::microseconds>(end-loaded).count()
                      << " peak_rss_kb=" << usage.ru_maxrss << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
