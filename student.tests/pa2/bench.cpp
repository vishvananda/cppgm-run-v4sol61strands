#include "preprocess/post/cursor.h"
#include <chrono>
#include <iostream>
#include <iterator>
#include <sys/resource.h>

int main() {
    std::ios::sync_with_stdio(false);
    const auto start = std::chrono::steady_clock::now();
    cppgm::SourceBuffer source(std::string(std::istreambuf_iterator<char>(std::cin), {}));
    const auto loaded = std::chrono::steady_clock::now();
    cppgm::IdentifierTable ids;
    cppgm::LexerOptions options; options.collect_literal_elements = true; options.convert_empty_character = true;
    cppgm::Lexer lexer(source, ids, options);
    cppgm::PostCursor cursor(lexer, ids);
    std::uint64_t checksum = 0;
    for (;;) {
        auto token = cursor.next();
        checksum += static_cast<unsigned>(token.kind) + token.range.begin + token.range.end + token.identifier + token.suffix;
        checksum += static_cast<unsigned>(token.type) + token.elements;
        if (token.numeric_data) for (std::size_t i = 0; i < token.numeric_prefix; ++i) checksum += static_cast<unsigned char>(token.numeric_data[i]);
        if (token.kind == cppgm::PostKind::array || token.kind == cppgm::PostKind::ud_string) {
            for (std::size_t i = 0; i < token.elements*token.width; ++i) checksum += token.units[i];
        } else for (auto byte : token.scalar) checksum += byte;
        if (token.kind == cppgm::PostKind::eof) break;
    }
    const auto done = std::chrono::steady_clock::now();
    struct rusage usage; getrusage(RUSAGE_SELF, &usage);
    const auto& p = cursor.metrics(); const auto& l = lexer.metrics();
    std::cerr << "checksum=" << checksum << " bytes=" << l.physical_bytes << " pp_tokens=" << l.tokens
              << " tokens=" << p.tokens << " numbers=" << p.numbers << " literal_elements=" << p.literal_elements
              << " sequences=" << p.string_sequences << " converted_bytes=" << p.converted_bytes
              << " max_sequence_elements=" << p.max_sequence_elements << " identifiers=" << ids.size()
              << " intern_probes=" << ids.probes() << " read_us="
              << std::chrono::duration_cast<std::chrono::microseconds>(loaded-start).count()
              << " convert_us=" << std::chrono::duration_cast<std::chrono::microseconds>(done-loaded).count()
              << " rss_kb=" << usage.ru_maxrss << '\n';
}
