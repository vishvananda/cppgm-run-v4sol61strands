// (C) 2013 CPPGM Foundation www.cppgm.org. All rights reserved.
// Cumulative PA1 lexer and typed PA2 conversion; starter token names preserved.
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sys/resource.h>
#include "preprocess/post/cursor.h"
#include "preprocess/post/render.h"

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
        cppgm::PostCursor cursor(lexer, identifiers, true);
        for (;;) {
            auto token = cursor.next();
            cppgm::render_posttoken(std::cout, token, identifiers);
            if (token.kind == cppgm::PostKind::eof) break;
        }
        if (std::getenv("CPPGM_POST_METRICS")) {
            const auto end = std::chrono::steady_clock::now();
            struct rusage usage; getrusage(RUSAGE_SELF, &usage);
            const auto& p = cursor.metrics(); const auto& l = lexer.metrics();
            std::cerr << "post bytes=" << l.physical_bytes << " pp_tokens=" << l.tokens
                      << " tokens=" << p.tokens << " numbers=" << p.numbers
                      << " literal_elements=" << p.literal_elements << " string_sequences=" << p.string_sequences
                      << " converted_bytes=" << p.converted_bytes << " max_sequence_elements=" << p.max_sequence_elements
                      << " identifiers=" << identifiers.size() << " intern_probes=" << identifiers.probes()
                      << " read_us=" << std::chrono::duration_cast<std::chrono::microseconds>(loaded-start).count()
                      << " convert_render_us=" << std::chrono::duration_cast<std::chrono::microseconds>(end-loaded).count()
                      << " peak_rss_kb=" << usage.ru_maxrss << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n'; return EXIT_FAILURE;
    }
}
