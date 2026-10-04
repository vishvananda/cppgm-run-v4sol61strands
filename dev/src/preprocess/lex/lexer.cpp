#include "preprocess/lex/lexer.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace cppgm {
namespace {

bool digit(int c) { return c >= '0' && c <= '9'; }
int hex(int c) {
    if (digit(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool space(int c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r'; }
int trigraph(int c) {
    switch (c) {
    case '=': return '#'; case '/': return '\\'; case '\'': return '^';
    case '(': return '['; case ')': return ']'; case '!': return '|';
    case '<': return '{'; case '>': return '}'; case '-': return '~';
    default: return -1;
    }
}
bool word_operator(const std::string& s) {
    // A fixed grammar inventory, not a TU-dependent name search.
    static const char* const words[] = {"new", "delete", "and", "and_eq", "bitand",
        "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq"};
    for (const char* word : words) if (s == word) return true;
    return false;
}
[[noreturn]] void invalid(const char* message) { throw std::runtime_error(message); }
}

Lexer::Lexer(const SourceBuffer& source, IdentifierTable& identifiers, LexerOptions options)
    : source_(source), identifiers_(identifiers), options_(options) {
    metrics_.physical_bytes = source.bytes.size();
    if (source.bytes.compare(0, 3, "\xEF\xBB\xBF") == 0) physical_ = 3;
}
Lexer::Character Lexer::decode(std::size_t offset) const {
    Character result{-1, {offset, line_, column_}, offset, false};
    if (offset == source_.bytes.size()) return result;
    unsigned c = static_cast<unsigned char>(source_.bytes[offset]);
    if (c < 0x80) { result.value = c; result.end = offset + 1; return result; }
    unsigned length, value, minimum;
    if (c >= 0xC2 && c <= 0xDF) { length = 2; value = c & 31; minimum = 0x80; }
    else if (c >= 0xE0 && c <= 0xEF) { length = 3; value = c & 15; minimum = 0x800; }
    else if (c >= 0xF0 && c <= 0xF4) { length = 4; value = c & 7; minimum = 0x10000; }
    else invalid("invalid UTF-8 leading byte");
    if (source_.bytes.size() - offset < length) invalid("incomplete UTF-8 sequence");
    for (unsigned i = 1; i < length; ++i) {
        unsigned next = static_cast<unsigned char>(source_.bytes[offset + i]);
        if ((next & 0xC0) != 0x80) invalid("invalid UTF-8 continuation byte");
        value = (value << 6) | (next & 63);
    }
    if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
        invalid("invalid UTF-8 scalar value");
    result.value = static_cast<int>(value); result.end = offset + length;
    return result;
}
Lexer::Character Lexer::phase1(std::size_t offset) const {
    Character c = decode(offset);
    if (c.value == '?' && source_.bytes.size() - offset >= 3
        && source_.bytes[offset + 1] == '?') {
        int replacement = trigraph(static_cast<unsigned char>(source_.bytes[offset + 2]));
        if (replacement != -1) { c.value = replacement; c.end = offset + 3; }
    }
    // Recognize complete UCNs only, after trigraph replacement. Odd preceding
    // backslashes protect escaped backslashes in ordinary string spellings.
    if (c.value == '\\' && !slash_odd_ && c.end < source_.bytes.size()) {
        char prefix = source_.bytes[c.end];
        unsigned length = prefix == 'u' ? 4 : prefix == 'U' ? 8 : 0;
        if (length && source_.bytes.size() - c.end >= length + 1) {
            std::uint32_t value = 0;
            bool complete = true;
            for (unsigned i = 0; i < length; ++i) {
                int h = hex(static_cast<unsigned char>(source_.bytes[c.end + 1 + i]));
                if (h < 0) { complete = false; break; }
                value = (value << 4) | h;
            }
            if (complete) {
                if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
                    invalid("invalid universal character value");
                c.value = static_cast<int>(value); c.end += length + 1; c.universal = true;
            }
        }
    }
    return c;
}
void Lexer::advance(std::size_t end) {
    while (physical_ < end) {
        if (source_.bytes[physical_++] == '\n') { ++line_; column_ = 1; }
        else ++column_; // physical UTF-8 byte column, not display width.
    }
}
Lexer::Character Lexer::translated() {
    for (;;) {
        Character c = phase1(physical_);
        if (c.value == -1) {
            if (!final_newline_ && (ended_splice_ || (last_ != -1 && last_ != '\n'))) {
                final_newline_ = true; c.value = '\n'; last_ = '\n';
            }
            return c;
        }
        advance(c.end);
        ++metrics_.decoded_characters;
        if (c.value == '\\' && !c.universal) {
            // A speculative following UCN sees this backslash's logical escape
            // parity. Otherwise an escaped backslash can decode (or reject) a
            // UCN before the first backslash is published to the cursor.
            const bool previous_parity = slash_odd_;
            slash_odd_ = !previous_parity;
            Character following = phase1(physical_);
            slash_odd_ = previous_parity;
            if (following.value == '\n' && !following.universal) {
                advance(following.end); ++metrics_.decoded_characters;
                ended_splice_ = true;
                // Splices do not reset escape parity in the logical stream.
                continue;
            }
        }
        ended_splice_ = false;
        slash_odd_ = c.value == '\\' && !c.universal ? !slash_odd_ : false;
        last_ = c.value;
        return c;
    }
}
const Lexer::Character& Lexer::peek(std::size_t n) {
    if (n >= lookahead_.size()) invalid("lexer lookahead bound exceeded");
    while (count_ <= n) lookahead_[count_++] = translated();
    return lookahead_[n];
}
Lexer::Character Lexer::take(bool append, bool literal_data) {
    Character c = peek();
    // N3485 [lex.charset]/2: basic/control UCNs are allowed only in literal
    // character sequences. Validate on consumption, not speculative lookahead.
    if (c.universal && !literal_data && c.value < 0xA0
        && c.value != 0x24 && c.value != 0x40 && c.value != 0x60)
        invalid("basic or control universal character outside literal");
    for (std::size_t i = 1; i < count_; ++i) lookahead_[i - 1] = lookahead_[i];
    --count_;
    consumed_end_ = c.end;
    if (append && c.value >= 0) {
        if (c.value < 128) spelling_ += static_cast<char>(c.value);
        else append_utf8(spelling_, c.value);
    }
    return c;
}
bool Lexer::matches(const char* text) {
    for (std::size_t i = 0; text[i]; ++i) if (peek(i).value != text[i]) return false;
    return true;
}
void Lexer::ordinary_literal(int quote) {
    take();
    std::size_t characters = 0;
    for (;;) {
        int c = peek().value;
        if (peek().universal) {
            ++characters;
            if (options_.collect_literal_elements) literal_elements_.push_back({static_cast<std::uint32_t>(c), false, false});
            take(true, true); continue;
        }
        if (c == -1 || c == '\n') invalid("unterminated ordinary literal");
        if (c == quote) { take(); break; }
        ++characters; take();
        if (c != '\\') {
            if (options_.collect_literal_elements) literal_elements_.push_back({static_cast<std::uint32_t>(c), false, false});
            continue;
        }
        c = peek().value;
        if (c == -1 || c == '\n') invalid("unterminated escape");
        const char* names = "'\"?\\abfnrtv";
        const char* values = "'\"?\\\a\b\f\n\r\t\v";
        const char* simple = c > 0 && c < 128 ? std::strchr(names, c) : nullptr;
        if (simple) {
            if (options_.collect_literal_elements) literal_elements_.push_back({static_cast<std::uint32_t>(values[simple-names]), false, false});
            take(); continue;
        }
        unsigned base = 8, limit = 3;
        if (c == 'x') {
            take(); base = 16; limit = ~0u;
            if (peek().universal || hex(peek().value) < 0) invalid("hex escape has no digits");
        } else if (c < '0' || c > '7') invalid("invalid escape sequence");
        std::uint32_t value = 0;
        bool overflow = false;
        for (unsigned i = 0; i < limit && !peek().universal; ++i) {
            int digit_value = hex(peek().value);
            if (digit_value < 0 || static_cast<unsigned>(digit_value) >= base) break;
            if (value > (UINT32_MAX-static_cast<unsigned>(digit_value))/base) overflow = true;
            value = value*base + digit_value;
            take();
        }
        if (options_.collect_literal_elements) literal_elements_.push_back({value, true, overflow});
    }
    if (quote == '\'' && !characters && !options_.convert_empty_character) invalid("empty character literal");
}
void Lexer::raw_literal() {
    Character quote = take();
    // Undo all phase 1/2 lookahead inside the quotes. Keep physical identities;
    // do not attempt to reconstruct original raw text from transformed spelling.
    count_ = 0; physical_ = quote.location.offset;
    line_ = quote.location.line; column_ = quote.location.column;
    advance(quote.end);
    std::string delimiter;
    unsigned delimiter_characters = 0;
    while (true) {
        Character c = decode(physical_);
        if (c.value == '(') { spelling_ += '('; advance(c.end); break; }
        // PA1 d-char excludes exactly SP/HT/VT/FF/LF, not CR. Its course
        // source set permits Unicode and other code points in raw delimiters.
        if (c.value == -1 || c.value == ')' || c.value == '\\' || c.value == '\n'
            || c.value == ' ' || c.value == '\t' || c.value == '\v' || c.value == '\f')
            invalid("invalid raw string delimiter");
        if (++delimiter_characters > 16) invalid("raw string delimiter is too long");
        delimiter.append(source_.bytes, physical_, c.end - physical_);
        spelling_.append(source_.bytes, physical_, c.end - physical_);
        advance(c.end); ++metrics_.decoded_characters;
    }
    const std::size_t body_begin = physical_;
    for (;;) {
        Character c = decode(physical_);
        if (c.value == -1) invalid("unterminated raw string literal");
        if (c.value == ')') {
            ++metrics_.raw_candidates;
            const std::size_t end = physical_ + 1 + delimiter.size();
            if (end < source_.bytes.size() && source_.bytes[end] == '"'
                && source_.bytes.compare(physical_ + 1, delimiter.size(), delimiter) == 0) {
                spelling_.append(source_.bytes, body_begin, end + 1 - body_begin);
                advance(end + 1); consumed_end_ = physical_;
                slash_odd_ = false; ended_splice_ = false; last_ = '"'; final_newline_ = false;
                return;
            }
        }
        if (options_.collect_literal_elements) literal_elements_.push_back({static_cast<std::uint32_t>(c.value), false, false});
        advance(c.end); ++metrics_.decoded_characters;
    }
}
void Lexer::suffix() {
    if (!identifier_initial(peek().value)) return;
    do { take(); } while (identifier_nondigit(peek().value) || digit(peek().value));
}
Token Lexer::finish(TokenKind kind, const Character& start, IdentifierId id) {
    ++metrics_.tokens; metrics_.spelling_bytes += spelling_.size();
    return Token{kind, {start.location.offset, consumed_end_}, start.location, id};
}
Token Lexer::next() {
    spelling_.clear(); literal_elements_.clear(); literal_end_ = 0;
    const Character start = peek();
    int c = start.value;
    if (c == -1) {
        consumed_end_ = start.end;
        return finish(TokenKind::eof, start);
    }
    if (c == '\n') {
        take(false); directive_ = Directive::start;
        return finish(TokenKind::newline, start);
    }
    if (space(c) || matches("//") || matches("/*")) {
        do {
            if (space(peek().value)) { take(false); continue; }
            if (matches("//")) {
                take(false); take(false);
                while (peek().value != '\n' && peek().value != -1) take(false);
                break;
            }
            if (!matches("/*")) break;
            take(false); take(false);
            while (!matches("*/")) {
                if (peek().value == -1) invalid("unterminated block comment");
                take(false);
            }
            take(false); take(false);
        } while (true);
        return finish(TokenKind::whitespace, start);
    }
    const Directive context = directive_;
    directive_ = Directive::ordinary;
    if (context == Directive::header && (c == '<' || c == '"')) {
        int close = c == '<' ? '>' : '"';
        take();
        if (peek().value == close) invalid("empty header name");
        while (peek().value != close) {
            if (peek().value == '\n' || peek().value == -1) invalid("unterminated header name");
            take();
        }
        take(); return finish(TokenKind::header, start);
    }
    // Test literal prefixes before identifier maximal munch. Never look through
    // a raw opening quote; its contents must not pass through phase1/2.
    static const char* const prefixes[] = {"u8R\"", "u8\"", "uR\"", "UR\"", "LR\"", "R\"",
        "u\"", "U\"", "L\"", "u'", "U'", "L'", "\"", "'"};
    if (c == 'u' || c == 'U' || c == 'L' || c == 'R' || c == '"' || c == '\'') {
        for (const char* prefix : prefixes) {
            if (!matches(prefix)) continue;
            std::size_t length = std::strlen(prefix);
            bool raw = length >= 2 && prefix[length - 2] == 'R';
            int quote = prefix[length - 1];
            for (std::size_t i = 1; i < length; ++i) take();
            if (raw) raw_literal(); else ordinary_literal(quote);
            std::size_t before = spelling_.size(); literal_end_ = before; suffix();
            bool ud = spelling_.size() != before;
            return finish(quote == '\'' ? (ud ? TokenKind::ud_character : TokenKind::character)
                : (ud ? TokenKind::ud_string : TokenKind::string), start);
        }
    }
    if (identifier_initial(c)) {
        do { take(); } while (identifier_nondigit(peek().value) || digit(peek().value));
        if (context == Directive::after_hash && spelling_ == "include") directive_ = Directive::header;
        if (word_operator(spelling_)) return finish(TokenKind::punctuator, start);
        return finish(TokenKind::identifier, start, identifiers_.intern(spelling_));
    }
    if (digit(c) || (c == '.' && digit(peek(1).value))) {
        int previous = take().value;
        while (true) {
            c = peek().value;
            if (digit(c) || identifier_nondigit(c) || c == '.'
                || ((c == '+' || c == '-') && (previous == 'e' || previous == 'E')))
                previous = take().value;
            else break;
        }
        return finish(TokenKind::number, start);
    }
    // The sole C++11 maximal-munch exception ([lex.pptoken]/3).
    if (matches("<::") && peek(3).value != ':' && peek(3).value != '>') {
        take(); return finish(TokenKind::punctuator, start);
    }
    // Dispatch by first character before checking the fixed grammar inventory.
    // Work is independent of translation-unit size and bounded by <=4 chars.
    const char* candidates = nullptr;
    switch (c) {
    case '%': candidates = "%:%: %: %> %= %"; break;
    case '<': candidates = "<<= << <= <: <% <"; break;
    case '>': candidates = ">>= >> >= >"; break;
    case '-': candidates = "->* -> -- -= -"; break;
    case '.': candidates = "... .* ."; break;
    case '#': candidates = "## #"; break;
    case ':': candidates = ":: :> :"; break;
    case '+': candidates = "++ += +"; break;
    case '*': candidates = "*= *"; break;
    case '/': candidates = "/= /"; break;
    case '^': candidates = "^= ^"; break;
    case '&': candidates = "&& &= &"; break;
    case '|': candidates = "|| |= |"; break;
    case '!': candidates = "!= !"; break;
    case '=': candidates = "== ="; break;
    default: break;
    }
    if (candidates) {
        while (*candidates) {
            const char* end = std::strchr(candidates, ' ');
            std::size_t length = end ? static_cast<std::size_t>(end - candidates) : std::strlen(candidates);
            bool match = true;
            for (std::size_t i = 0; i < length; ++i)
                if (peek(i).value != candidates[i]) { match = false; break; }
            if (match) {
                for (std::size_t i = 0; i < length; ++i) take();
                if (context == Directive::start && (spelling_ == "#" || spelling_ == "%:"))
                    directive_ = Directive::after_hash;
                return finish(TokenKind::punctuator, start);
            }
            if (!end) break;
            candidates = end + 1;
        }
    }
    take();
    bool single = c != 0 && c < 128 && std::strchr("{}[]();?~,", c);
    return finish(single ? TokenKind::punctuator : TokenKind::other, start);
}
} // namespace cppgm
