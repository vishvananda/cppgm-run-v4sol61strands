#include "preprocess/post/render.h"
#include <ostream>
namespace cppgm {
namespace {
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
            out.write(token.source.data(), token.numeric_prefix); out << '\n'; return;
        }
        out << (kind == PostKind::ud_string ? "string " : "character ");
    }
    if (kind == PostKind::array || kind == PostKind::ud_string) {
        out << "array of " << token.elements << ' ' << type_name(token.type) << ' ';
        hex(out, token.units, token.elements*token.width);
    } else {
        out << type_name(token.type) << ' '; hex(out, token.scalar.data(), token.width);
    }
    out << '\n';
}
}
