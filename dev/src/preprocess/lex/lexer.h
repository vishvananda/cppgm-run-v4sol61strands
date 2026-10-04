#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
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

class IdentifierTable {
    std::unordered_map<std::string, IdentifierId> ids_;
    std::vector<const std::string*> spellings_;
public:
    IdentifierId intern(const std::string& spelling);
    const std::string& spelling(IdentifierId id) const;
    std::size_t size() const { return spellings_.size(); }
};

struct LexerMetrics {
    std::size_t physical_bytes = 0, decoded_characters = 0;
    std::size_t tokens = 0, spelling_bytes = 0, raw_candidates = 0;
};

// Pull tokens, never a complete owning token vector. spelling() is borrowed
// until next(); identifiers retain stable identity in the caller-owned table.
class Lexer {
public:
    Lexer(const SourceBuffer& source, IdentifierTable& identifiers);
    Token next();
    const std::string& spelling() const { return spelling_; }
    const LexerMetrics& metrics() const { return metrics_; }
private:
    struct Character {
        int value;
        SourceLocation location;
        std::size_t end;
    };
    const SourceBuffer& source_;
    IdentifierTable& identifiers_;
    std::size_t physical_ = 0, line_ = 1, column_ = 1;
    std::array<Character, 8> lookahead_;
    std::size_t count_ = 0, consumed_end_ = 0;
    bool final_newline_ = false, slash_odd_ = false;
    int last_ = -1;
    enum class Directive { start, after_hash, header, ordinary };
    Directive directive_ = Directive::start;
    std::string spelling_;
    LexerMetrics metrics_;
    Character decode(std::size_t offset) const;
    Character phase1(std::size_t offset) const;
    void advance(std::size_t end);
    Character translated();
    const Character& peek(std::size_t n = 0);
    Character take(bool append = true);
    bool matches(const char* text);
    void raw_literal();
    void ordinary_literal(int quote);
    void suffix();
    Token finish(TokenKind kind, const Character& start, IdentifierId id = 0);
};


} // namespace cppgm
