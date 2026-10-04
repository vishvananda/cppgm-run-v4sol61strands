#pragma once
#include "preprocess/lex/lexer.h"
#include "preprocess/tokens/IPPTokenStream.h"

namespace cppgm {
// Compatibility rendering adapter; production phases consume Lexer::next().
inline void emit_token(const Token& token, const std::string& data, IPPTokenStream& out) {
    out.set_source_location(token.location.line, token.location.column);
    switch (token.kind) {
    case TokenKind::whitespace: out.emit_whitespace_sequence(); break;
    case TokenKind::newline: out.emit_new_line(); break;
    case TokenKind::header: out.emit_header_name(data); break;
    case TokenKind::identifier: out.emit_identifier(data); break;
    case TokenKind::number: out.emit_pp_number(data); break;
    case TokenKind::character: out.emit_character_literal(data); break;
    case TokenKind::ud_character: out.emit_user_defined_character_literal(data); break;
    case TokenKind::string: out.emit_string_literal(data); break;
    case TokenKind::ud_string: out.emit_user_defined_string_literal(data); break;
    case TokenKind::punctuator: out.emit_preprocessing_op_or_punc(data); break;
    case TokenKind::other: out.emit_non_whitespace_char(data); break;
    case TokenKind::eof: out.emit_eof(); break;
    }
}
}
