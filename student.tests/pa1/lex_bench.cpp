#include "preprocess/lex/lexer.h"
#include <chrono>
#include <iostream>
#include <iterator>
#include <sys/resource.h>

int main() {
    std::ios::sync_with_stdio(false);
    const auto begin = std::chrono::steady_clock::now();
    cppgm::SourceBuffer source(std::string(std::istreambuf_iterator<char>(std::cin), {}));
    const auto loaded = std::chrono::steady_clock::now();
    cppgm::IdentifierTable ids;
    cppgm::Lexer lexer(source, ids);
    std::uint64_t checksum = 0;
    for (;;) {
        auto token = lexer.next();
        checksum += static_cast<unsigned>(token.kind) + token.range.begin + token.range.end + token.identifier;
        if (token.kind == cppgm::TokenKind::eof) break;
    }
    const auto done = std::chrono::steady_clock::now();
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    const auto& m = lexer.metrics();
    std::cerr << "checksum=" << checksum << " bytes=" << m.physical_bytes
              << " tokens=" << m.tokens << " decoded=" << m.decoded_characters
              << " names=" << ids.size() << " intern_probes=" << ids.probes() << " spelling_slabs=" << ids.slabs() << " raw_candidates=" << m.raw_candidates
              << " read_us=" << std::chrono::duration_cast<std::chrono::microseconds>(loaded-begin).count()
              << " lex_us=" << std::chrono::duration_cast<std::chrono::microseconds>(done-loaded).count()
              << " rss_kb=" << usage.ru_maxrss << '\n';
}
