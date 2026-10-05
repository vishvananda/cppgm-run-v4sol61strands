#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include "preprocess/lex/unicode.h"

namespace cppgm {

// The source outlives its cursor and any retained ranges. No normalized TU copy.
struct SourceBuffer {
    const std::string bytes;
    explicit SourceBuffer(std::string input) : bytes(std::move(input)) {}
};
struct SourceLocation {
    std::size_t offset, line, column;
    // Zero for standalone lexer tools; preprocessing assigns a TU source ID.
    std::uint32_t file;
};
struct SourceRange { std::size_t begin, end; };
enum class TokenKind : unsigned char {
    whitespace, newline, header, identifier, number, character, ud_character,
    string, ud_string, punctuator, other, eof
};
using IdentifierId = std::uint32_t;
struct Token {
    TokenKind kind;
    SourceRange range;
    SourceLocation location;
    IdentifierId identifier;
};

struct IdentifierSpelling {
    const char* data;
    std::size_t size;
    bool equals(const std::string& spelling) const;
};

// Dense open-addressed IDs; spelling bytes have TU-owned slab lifetime. Rehash
// moves compact IDs only, never spelling bytes or one heap node per name.
class IdentifierTable {
    struct Entry { IdentifierSpelling spelling; std::uint64_t hash; };
    std::vector<IdentifierId> slots_;
    std::vector<Entry> entries_;
    std::vector<std::unique_ptr<char[]>> slabs_;
    std::size_t slab_used_ = 0, slab_capacity_ = 0, probes_ = 0;
    void grow();
public:
    IdentifierTable() = default;
    IdentifierTable(const IdentifierTable&) = delete;
    IdentifierTable& operator=(const IdentifierTable&) = delete;
    IdentifierId intern(const std::string& spelling);
    IdentifierSpelling spelling(IdentifierId id) const;
    std::size_t size() const { return entries_.size(); }
    std::size_t probes() const { return probes_; }
    std::size_t slabs() const { return slabs_.size(); }
};

// Literal elements retain numeric-escape provenance for phase 6.
struct LiteralElement { std::uint32_t value; bool numeric, overflow; };

struct LexerMetrics {
    std::size_t physical_bytes = 0, decoded_characters = 0;
    std::size_t tokens = 0, spelling_bytes = 0, raw_candidates = 0;
};

// Pull tokens, never a complete owning token vector. spelling() is borrowed
// until next(); identifiers retain stable identity in the caller-owned table.
struct LexerOptions {
    bool collect_literal_elements = false;
    // PA2 converts a closed empty character token to invalid and continues;
    // PA1 still reports a phase-3 error, preserving its lexical contract.
    bool convert_empty_character = false;
    // Generated PP tokens have already undergone phases 1/2.
    bool generated_token = false;
};

// Common structured pull interface; views remain valid until next().
class PPSource {
public:
    virtual ~PPSource() {}
    virtual Token next() = 0;
    virtual const std::string& spelling() const = 0;
    virtual const std::vector<LiteralElement>& literal_elements() const = 0;
    virtual std::size_t literal_end() const = 0;
    virtual std::size_t literal_physical_end() const = 0;
    virtual SourceLocation literal_suffix_location() const = 0;
};

class Lexer : public PPSource {
public:
    Lexer(const SourceBuffer& source, IdentifierTable& identifiers, LexerOptions options = LexerOptions());
    Token next();
    const std::string& spelling() const { return spelling_; }
    const LexerMetrics& metrics() const { return metrics_; }
    const std::vector<LiteralElement>& literal_elements() const { return literal_elements_; }
    std::size_t literal_end() const { return literal_end_; }
    std::size_t literal_physical_end() const { return literal_physical_end_; }
    SourceLocation literal_suffix_location() const { return literal_suffix_location_; }
private:
    struct Character {
        int value;
        SourceLocation location;
        std::size_t end;
        bool universal; // Provenance survives phase 1; literal data is not syntax.
    };
    const SourceBuffer& source_;
    IdentifierTable& identifiers_;
    std::size_t physical_ = 0, line_ = 1, column_ = 1;
    std::array<Character, 8> lookahead_;
    std::size_t count_ = 0, consumed_end_ = 0;
    bool final_newline_ = false, slash_odd_ = false, ended_splice_ = false;
    int last_ = -1;
    enum class Directive { start, after_hash, header, ordinary };
    Directive directive_ = Directive::start;
    std::string spelling_;
    LexerMetrics metrics_;
    LexerOptions options_;
    std::vector<LiteralElement> literal_elements_;
    std::size_t literal_end_ = 0, literal_physical_end_ = 0;
    SourceLocation literal_suffix_location_ = {0, 0, 0};
    Character decode(std::size_t offset) const;
    Character phase1(std::size_t offset) const;
    void advance(std::size_t end);
    Character translated();
    const Character& peek(std::size_t n = 0);
    Character take(bool append = true, bool literal_data = false);
    bool matches(const char* text);
    void raw_literal();
    void ordinary_literal(int quote);
    void suffix();
    Token finish(TokenKind kind, const Character& start, IdentifierId id = 0);
};


} // namespace cppgm
