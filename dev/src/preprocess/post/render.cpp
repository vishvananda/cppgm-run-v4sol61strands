#include "preprocess/post/render.h"
#include <ostream>
#include <sstream>
#include <cstring>
namespace cppgm {
namespace {
// PA2 explicitly requires the starter stream scans for bit-perfect debug
// output. Keep their allocations out of production typed numeric conversion.
// (C) 2013 CPPGM Foundation www.cppgm.org. All rights reserved.
float PA2Decode_float(const std::string& s) { std::istringstream in(s); float x = 0; in >> x; return x; }
double PA2Decode_double(const std::string& s) { std::istringstream in(s); double x = 0; in >> x; return x; }
long double PA2Decode_long_double(const std::string& s) { std::istringstream in(s); long double x = 0; in >> x; return x; }
void hex(std::ostream& out, const unsigned char* bytes, std::size_t size) {
    static const char digits[] = "0123456789ABCDEF";
    // One buffered insertion instead of per-byte formatted iostream calls.
    std::string text(size*2, '0');
    for (std::size_t i = 0; i < size; ++i) {
        text[2*i] = digits[bytes[i]>>4]; text[2*i+1] = digits[bytes[i]&15];
    }
    out << text;
}
}
void render_posttoken(std::ostream& out, const PostToken& token, const IdentifierTable& ids) {
    const auto kind = token.kind;
    if (kind == PostKind::eof) { out << "eof\n"; return; }
    if (kind == PostKind::invalid) { out << "invalid " << token.source << '\n'; return; }
    if (kind == PostKind::simple) { out << "simple " << token.source << ' ' << simple_name(token.simple) << '\n'; return; }
    if (kind == PostKind::identifier) { out << "identifier " << token.source << '\n'; return; }
    const bool ud = kind == PostKind::ud_integer || kind == PostKind::ud_floating || kind == PostKind::ud_character || kind == PostKind::ud_string;
    out << (ud ? "user-defined-literal " : "literal ") << token.source << ' ';
    if (ud) {
        auto suffix = ids.spelling(token.suffix); out.write(suffix.data, suffix.size); out << ' ';
        if (kind == PostKind::ud_integer || kind == PostKind::ud_floating) {
            out << (kind == PostKind::ud_integer ? "integer " : "floating ");
            out.write(token.numeric_data, token.numeric_prefix); out << '\n'; return;
        }
        out << (kind == PostKind::ud_string ? "string " : "character ");
    }
    if (kind == PostKind::array || kind == PostKind::ud_string) {
        out << "array of " << token.elements << ' ' << type_name(token.type) << ' ';
        hex(out, token.units, token.elements*token.width);
    } else {
        out << type_name(token.type) << ' ';
        std::array<unsigned char, 16> bytes = token.scalar;
        if (token.type == FundamentalType::FT_FLOAT) {
            float value = PA2Decode_float(token.source); std::memcpy(bytes.data(), &value, sizeof(value));
        } else if (token.type == FundamentalType::FT_DOUBLE) {
            double value = PA2Decode_double(token.source); std::memcpy(bytes.data(), &value, sizeof(value));
        } else if (token.type == FundamentalType::FT_LONG_DOUBLE) {
            long double value = PA2Decode_long_double(token.source); std::memcpy(bytes.data(), &value, 10);
        }
        hex(out, bytes.data(), token.width);
    }
    out << '\n';
}
}
