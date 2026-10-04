#include "preprocess/post/cursor.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace cppgm;
int main() {
    SourceBuffer source("int id = 42_u; \"\\x3c0\" u\"😀\"_π; id; 'a'; \"\\u0022\\u005c\";");
    IdentifierTable ids;
    LexerOptions options; options.collect_literal_elements = true; options.convert_empty_character = true;
    Lexer lexer(source, ids, options);
    PostCursor cursor(lexer, ids);
    auto keyword = cursor.next(); assert(keyword.kind == PostKind::simple && keyword.simple == SimpleKind::KW_INT);
    auto name = cursor.next(); assert(name.kind == PostKind::identifier && name.identifier);
    assert(cursor.next().simple == SimpleKind::OP_ASS);
    auto number = cursor.next(); assert(number.kind == PostKind::ud_integer && number.numeric_prefix == 2);
    assert(ids.spelling(number.suffix).equals("_u"));
    assert(cursor.next().simple == SimpleKind::OP_SEMICOLON);
    auto string = cursor.next();
    assert(string.kind == PostKind::ud_string && string.type == FundamentalType::FT_CHAR16_T && string.elements == 4);
    const unsigned char expected[] = {0xC0, 0x03, 0x3D, 0xD8, 0x00, 0xDE, 0, 0};
    assert(std::memcmp(string.units, expected, 8) == 0 && string.source.empty());
    assert(ids.spelling(string.suffix).equals("_π"));
    assert(cursor.next().simple == SimpleKind::OP_SEMICOLON);
    assert(cursor.next().identifier == name.identifier);
    assert(cursor.next().simple == SimpleKind::OP_SEMICOLON);
    auto character = cursor.next(); assert(character.type == FundamentalType::FT_CHAR && character.scalar[0] == 'a');
    assert(cursor.next().simple == SimpleKind::OP_SEMICOLON);
    auto escaped = cursor.next(); assert(escaped.kind == PostKind::array && escaped.elements == 3);
    assert(escaped.units[0] == '"' && escaped.units[1] == '\\');
    assert(cursor.next().simple == SimpleKind::OP_SEMICOLON);
    assert(cursor.next().kind == PostKind::eof);
    assert(cursor.metrics().numbers == 1 && cursor.metrics().string_sequences == 2);
    assert(cursor.metrics().literal_elements == 5);
    assert(keyword.range.begin == 0 && keyword.range.end == 3 && keyword.source.empty());
    std::cout << "typed cursor ownership/provenance controls passed\n";
}
