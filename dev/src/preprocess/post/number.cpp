#include "preprocess/post/number.h"
#include <climits>
#include <cstring>
#include <sstream>

namespace cppgm {
namespace {
bool decimal(char c) { return c >= '0' && c <= '9'; }
int digit(char c) {
    if (decimal(c)) return c-'0';
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    return -1;
}
// Spelling was already validated as UTF-8 by Lexer. No second lexer/parser.
bool suffix(const std::string& s, std::size_t begin) {
    if (begin == s.size() || s[begin] != '_') return false;
    for (std::size_t i = begin; i < s.size();) {
        unsigned c = static_cast<unsigned char>(s[i++]);
        if (c >= 128) {
            unsigned count = c < 224 ? 1 : c < 240 ? 2 : 3;
            c &= count == 1 ? 31 : count == 2 ? 15 : 7;
            while (count--) c = (c<<6) | (static_cast<unsigned char>(s[i++]) & 63);
        }
        if (!identifier_nondigit(c) && !(c >= '0' && c <= '9')) return false;
    }
    return true;
}
struct IntegerSuffix { bool valid, unsign; unsigned longs; };
IntegerSuffix integer_suffix(const std::string& s, std::size_t begin) {
    IntegerSuffix result = {false, false, 0};
    std::size_t i = begin;
    if (i < s.size() && (s[i] == 'u' || s[i] == 'U')) { result.unsign = true; ++i; }
    if (i < s.size() && (s[i] == 'l' || s[i] == 'L')) {
        char c = s[i++]; result.longs = 1;
        if (i < s.size() && s[i] == c) { ++i; result.longs = 2; }
    }
    if (!result.unsign && i < s.size() && (s[i] == 'u' || s[i] == 'U')) {
        result.unsign = true; ++i;
    }
    result.valid = i == s.size(); return result;
}
// Required PA2 output scan: these are the starter PA2Decode algorithms.
float PA2Decode_float(const std::string& s) { std::istringstream in(s); float x = 0; in >> x; return x; }
double PA2Decode_double(const std::string& s) { std::istringstream in(s); double x = 0; in >> x; return x; }
long double PA2Decode_long_double(const std::string& s) { std::istringstream in(s); long double x = 0; in >> x; return x; }
template<class T> void store(PostToken& token, T value) {
    token.width = sizeof(T); std::memcpy(token.scalar.data(), &value, sizeof(T));
}
}

void convert_number(const std::string& s, IdentifierTable& ids, PostToken& token) {
    std::size_t i = 0, digits_begin = 0;
    unsigned base = 10;
    bool floating = false, octal_bad = false;
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16; i = digits_begin = 2;
        while (i < s.size() && digit(s[i]) >= 0) ++i;
        if (i == digits_begin) return;
    } else {
        while (i < s.size() && decimal(s[i])) ++i;
        if (i < s.size() && s[i] == '.') {
            floating = true; ++i;
            while (i < s.size() && decimal(s[i])) ++i;
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            // A failed exponent is not a floating prefix. It can only be a
            // reserved identifier suffix (rejected below), never a builtin.
            std::size_t exponent = i+1;
            if (exponent < s.size() && (s[exponent] == '+' || s[exponent] == '-')) ++exponent;
            std::size_t start = exponent;
            while (exponent < s.size() && decimal(s[exponent])) ++exponent;
            if (exponent != start) { floating = true; i = exponent; }
        }
        if (!floating && s[0] == '0') {
            base = 8;
            for (std::size_t j = 0; j < i; ++j) if (s[j] > '7') octal_bad = true;
        }
    }
    if (suffix(s, i)) {
        if (octal_bad) return;
        token.suffix = ids.intern(s.substr(i)); token.numeric_prefix = i;
        token.kind = floating ? PostKind::ud_floating : PostKind::ud_integer;
        return; // Raw UD numeric spellings have no builtin type/range limit.
    }
    if (floating) {
        char type = 0;
        if (i != s.size()) {
            if (i+1 != s.size() || (s[i] != 'f' && s[i] != 'F' && s[i] != 'l' && s[i] != 'L')) return;
            type = s[i];
        }
        token.kind = PostKind::scalar;
        if (type == 'f' || type == 'F') {
            token.type = FundamentalType::FT_FLOAT; store(token, PA2Decode_float(s));
        } else if (type == 'l' || type == 'L') {
            token.type = FundamentalType::FT_LONG_DOUBLE;
            long double value = PA2Decode_long_double(s);
            // x87's ten value bytes plus deterministic six ABI padding bytes.
            token.width = 16; std::memcpy(token.scalar.data(), &value, 10);
        } else { token.type = FundamentalType::FT_DOUBLE; store(token, PA2Decode_double(s)); }
        return;
    }
    IntegerSuffix spec = integer_suffix(s, i);
    if (!spec.valid || octal_bad) return;
    std::uint64_t value = 0;
    for (std::size_t j = digits_begin; j < i; ++j) {
        unsigned d = digit(s[j]);
        if (value > (UINT64_MAX-d)/base) return;
        value = value*base+d;
    }
    // Candidate order is the C++11 [lex.icon] table, on the course LP64 ABI.
    const bool nondecimal = base != 10;
    FundamentalType chosen;
    std::size_t width;
    if (!spec.longs && !spec.unsign && value <= INT32_MAX) {
        chosen = FundamentalType::FT_INT; width = 4;
    } else if (!spec.longs && (spec.unsign || nondecimal) && value <= UINT32_MAX) {
        chosen = FundamentalType::FT_UNSIGNED_INT; width = 4;
    } else if (!spec.unsign && value <= INT64_MAX) {
        chosen = spec.longs == 2 ? FundamentalType::FT_LONG_LONG_INT : FundamentalType::FT_LONG_INT; width = 8;
    } else if (spec.unsign || nondecimal) {
        chosen = spec.longs == 2 ? FundamentalType::FT_UNSIGNED_LONG_LONG_INT : FundamentalType::FT_UNSIGNED_LONG_INT; width = 8;
    } else return;
    token.kind = PostKind::scalar; token.type = chosen; token.width = width;
    for (std::size_t j = 0; j < width; ++j) token.scalar[j] = value >> (8*j);
}
}
