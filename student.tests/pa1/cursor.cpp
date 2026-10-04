#include "preprocess/lex/lexer.h"
#include <cassert>
#include <iostream>

using namespace cppgm;

int main() {
    SourceBuffer source("alpha\\\nbeta\nR\"(raw\ntext)\" α alpha\\\nbeta");
    IdentifierTable ids;
    Lexer cursor(source, ids);
    Token first = cursor.next();
    assert(first.kind == TokenKind::identifier && cursor.spelling() == "alphabeta");
    assert(first.range.begin == 0 && first.range.end == 11);
    assert(first.location.line == 1 && first.location.column == 1);
    assert(cursor.next().kind == TokenKind::newline);
    Token raw = cursor.next();
    assert(raw.kind == TokenKind::string && raw.location.line == 3);
    assert(cursor.spelling() == "R\"(raw\ntext)\"");
    assert(source.bytes.substr(raw.range.begin, raw.range.end - raw.range.begin) == cursor.spelling());
    assert(cursor.next().kind == TokenKind::whitespace);
    Token greek = cursor.next();
    assert(greek.location.line == 4 && greek.location.column == 8);
    assert(cursor.next().kind == TokenKind::whitespace);
    Token last = cursor.next();
    assert(last.identifier == first.identifier);
    assert(last.location.line == 4 && last.location.column == 11);
    assert(cursor.next().kind == TokenKind::newline);
    Token eof = cursor.next();
    assert(eof.kind == TokenKind::eof && eof.range.begin == source.bytes.size());
    // UCN provenance: offsets span the original spelling even when decoded
    // data is a delimiter, newline, NUL or backslash.
    SourceBuffer literal("\"\\u0022\\u005c\\u000A\\u0000\" next");
    Lexer literal_cursor(literal, ids);
    Token lit = literal_cursor.next();
    assert(lit.kind == TokenKind::string && lit.range.begin == 0 && lit.range.end == 26);
    assert(literal_cursor.spelling() == std::string("\"\"\\\n\0\"", 6));
    assert(literal_cursor.next().kind == TokenKind::whitespace);
    Token after = literal_cursor.next();
    assert(after.location.line == 1 && after.location.column == 28);
    assert(after.range.begin == 27 && literal_cursor.spelling() == "next");

    // Literal lookahead resumes phase translation after the physical raw body.
    SourceBuffer equivalent("α \\u03B1 alpha\\\nbeta");
    Lexer equivalent_cursor(equivalent, ids);
    assert(equivalent_cursor.next().identifier == greek.identifier);
    equivalent_cursor.next();
    assert(equivalent_cursor.next().identifier == greek.identifier);
    equivalent_cursor.next();
    assert(equivalent_cursor.next().identifier == first.identifier);
    equivalent_cursor.next();
    Token end1 = equivalent_cursor.next(), end2 = equivalent_cursor.next();
    assert(end1.kind == TokenKind::eof && end2.kind == TokenKind::eof);
    assert(end1.range.begin == end2.range.begin && end1.range.end == end2.range.end);

    const char* retained = ids.spelling(first.identifier).data;
    for (unsigned n = 0; n < 100000; ++n) ids.intern("unique" + std::to_string(n));
    assert(retained == ids.spelling(first.identifier).data && ids.spelling(first.identifier).equals("alphabeta"));
    assert(ids.intern("α") == greek.identifier);
    assert(ids.slabs() < 100 && ids.probes() < 1000000);
    std::string huge(100000, 'x');
    IdentifierId huge_id = ids.intern(huge);
    assert(ids.spelling(huge_id).equals(huge) && ids.intern(huge) == huge_id);

    // Each input byte has constant translation work; repeated identifiers do not
    // grow the table, and cursor storage is independent of token count.
    for (unsigned size : {1000u, 10000u, 100000u}) {
        std::string input;
        for (unsigned n = 0; n < size; ++n) input += "name += 12e+3; R\"d(x))dx)d\"\n";
        SourceBuffer scale(std::move(input));
        IdentifierTable names;
        Lexer lex(scale, names);
        while (lex.next().kind != TokenKind::eof) {}
        const auto& m = lex.metrics();
        assert(names.size() == 1);
        assert(m.tokens == size * 9 + 1);
        assert(m.decoded_characters <= scale.bytes.size() * 2);
        assert(m.raw_candidates == size * 3);
        std::cout << "scale bytes=" << scale.bytes.size() << " tokens=" << m.tokens
                  << " decoded=" << m.decoded_characters << " raw_candidates=" << m.raw_candidates << '\n';
    }
    // Every Annex E boundary, UTF-8 length transition and scalar maximum.
    for (int value : {0, 0x7f, 0x80, 0x7ff, 0x800, 0x2ff, 0x300, 0x36f, 0x370,
                     0x1dbf, 0x1dc0, 0x1dff, 0x1e00, 0x20cf, 0x20d0, 0x20ff, 0x2100,
                     0xfe1f, 0xfe20, 0xfe2f, 0xfe30, 0xd7ff, 0xe000, 0xffff,
                     0x10000, 0x1fffd, 0x1fffe, 0xefffd, 0xefffe, 0x10ffff}) {
        std::string encoded;
        append_utf8(encoded, value);
        SourceBuffer unicode(encoded);
        IdentifierTable names;
        Lexer lex(unicode, names);
        Token token = lex.next();
        assert(token.kind == (identifier_initial(value) ? TokenKind::identifier : TokenKind::other));
        assert(lex.spelling() == encoded);
    }
    SourceBuffer bom("\xef\xbb\xbf");
    Lexer empty(bom, ids);
    Token end = empty.next();
    assert(end.kind == TokenKind::eof && end.range.begin == end.range.end);
    std::cout << "typed cursor / identity / source mapping controls passed\n";
}
