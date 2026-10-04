#include "preprocess/post/cursor.h"
#include "preprocess/post/number.h"
#include <algorithm>

namespace cppgm {
namespace {
enum class Encoding : unsigned char { ordinary, utf8, utf16, utf32, wide };
Encoding encoding(const std::string& s) {
    if (s[0] == 'u') return s[1] == '8' ? Encoding::utf8 : Encoding::utf16;
    if (s[0] == 'U') return Encoding::utf32;
    if (s[0] == 'L') return Encoding::wide;
    return Encoding::ordinary;
}
FundamentalType type(Encoding e) {
    switch (e) {
    case Encoding::utf16: return FundamentalType::FT_CHAR16_T;
    case Encoding::utf32: return FundamentalType::FT_CHAR32_T;
    case Encoding::wide: return FundamentalType::FT_WCHAR_T;
    default: return FundamentalType::FT_CHAR;
    }
}
unsigned width(Encoding e) {
    return e == Encoding::utf16 ? 2 : e == Encoding::utf32 || e == Encoding::wide ? 4 : 1;
}
bool scalar(std::uint32_t c) { return c < 0x110000 && (c < 0xD800 || c >= 0xE000); }
void unit(std::vector<unsigned char>& bytes, std::uint32_t value, unsigned size) {
    for (unsigned i = 0; i < size; ++i) bytes.push_back(value >> (8*i));
}
bool encode(const LiteralElement& element, Encoding e, std::vector<unsigned char>& bytes) {
    const std::uint32_t c = element.value;
    const unsigned size = width(e);
    if (element.overflow) return false;
    if (element.numeric) {
        if ((size == 1 && c > 255) || (size == 2 && c > 65535)) return false;
        unit(bytes, c, size); return true;
    }
    if (!scalar(c)) return false;
    if (size == 4 || (size == 2 && c < 65536)) unit(bytes, c, size);
    else if (size == 2) {
        unit(bytes, 0xD800 + ((c-0x10000)>>10), 2);
        unit(bytes, 0xDC00 + ((c-0x10000)&1023), 2);
    } else if (c < 128) unit(bytes, c, 1);
    else if (c < 2048) {
        unit(bytes, 0xC0|(c>>6), 1); unit(bytes, 0x80|(c&63), 1);
    } else if (c < 65536) {
        unit(bytes, 0xE0|(c>>12), 1); unit(bytes, 0x80|((c>>6)&63), 1); unit(bytes, 0x80|(c&63), 1);
    } else {
        unit(bytes, 0xF0|(c>>18), 1); unit(bytes, 0x80|((c>>12)&63), 1);
        unit(bytes, 0x80|((c>>6)&63), 1); unit(bytes, 0x80|(c&63), 1);
    }
    return true;
}
bool is_string(TokenKind kind) { return kind == TokenKind::string || kind == TokenKind::ud_string; }
}
void PostCursor::advance() {
    do { current_ = lexer_.next(); } while (current_.kind == TokenKind::whitespace || (!controlling_ && current_.kind == TokenKind::newline));
    ready_ = true;
}
void PostCursor::character(PostToken& result) {
    const auto& s = lexer_.spelling();
    const auto& elements = lexer_.literal_elements();
    metrics_.literal_elements += elements.size();
    if (elements.size() != 1 || elements[0].overflow || !scalar(elements[0].value)) return;
    const auto c = elements[0].value;
    const Encoding e = encoding(s);
    if (e == Encoding::utf16 && c >= 65536) return;
    if (current_.kind == TokenKind::ud_character) {
        const std::size_t end = lexer_.literal_end();
        if (s[end] != '_') return;
        result.suffix = identifiers_.intern(s.substr(end));
        result.kind = PostKind::ud_character;
    } else result.kind = PostKind::scalar;
    result.type = type(e); result.width = width(e);
    if (e == Encoding::ordinary && c > 127) { result.type = FundamentalType::FT_INT; result.width = 4; }
    for (std::size_t i = 0; i < result.width; ++i) result.scalar[i] = c >> (8*i);
}
void PostCursor::strings(PostToken& result) {
    sequence_elements_.clear();
    auto& elements = sequence_elements_;
    Encoding selected = Encoding::ordinary;
    bool valid = true, first = true;
    ++metrics_.string_sequences;
    do {
        const auto& s = lexer_.spelling();
        if (!first && render_source_) result.source += ' ';
        if (render_source_) result.source += s;
        first = false;
        const Encoding e = encoding(s);
        if (e != Encoding::ordinary) {
            if (selected != Encoding::ordinary && selected != e) valid = false;
            selected = e;
        }
        if (current_.kind == TokenKind::ud_string) {
            const std::size_t end = lexer_.literal_end();
            if (s[end] != '_') valid = false;
            IdentifierId suffix = identifiers_.intern(s.substr(end));
            if (result.suffix && result.suffix != suffix) valid = false;
            result.suffix = suffix;
        }
        const auto& part = lexer_.literal_elements();
        metrics_.literal_elements += part.size();
        elements.insert(elements.end(), part.begin(), part.end());
        result.range.end = current_.range.end;
        advance();
    } while (is_string(current_.kind));
    metrics_.max_sequence_elements = std::max(metrics_.max_sequence_elements, elements.size());
    if (!valid) return;
    result.type = type(selected); result.width = width(selected);
    sequence_units_.clear();
    sequence_units_.reserve(elements.size()*result.width + result.width);
    for (const auto& element : elements) if (!encode(element, selected, sequence_units_)) {
        sequence_units_.clear(); return;
    }
    unit(sequence_units_, 0, result.width);
    result.units = sequence_units_.data();
    result.elements = sequence_units_.size()/result.width;
    metrics_.converted_bytes += sequence_units_.size();
    result.kind = result.suffix ? PostKind::ud_string : PostKind::array;
}
PostToken PostCursor::next() {
    PostToken result;
    if (pending_identifier_) {
        result.kind = PostKind::identifier; result.identifier = pending_identifier_;
        result.range = pending_range_; result.location = pending_location_;
        if (render_source_) {
            auto spelling = identifiers_.spelling(pending_identifier_);
            result.source.assign(spelling.data, spelling.size);
        }
        pending_identifier_ = 0; ++metrics_.tokens; after_operator_ = false;
        return result;
    }
    if (!ready_) advance();
    result.range = current_.range; result.location = current_.location;
    const auto& s = lexer_.spelling();
    // C++11 literal-operator-id also permits reserved suffix identifiers. The
    // operator's unprefixed, non-raw empty string and adjacent identifier
    // are separate tokens, regardless of underscore/reserved suffix spelling.
    if (after_operator_ && current_.kind == TokenKind::ud_string &&
        lexer_.literal_end() == 2 && s.compare(0, 2, "\"\"") == 0) {
        pending_identifier_ = identifiers_.intern(s.substr(lexer_.literal_end()));
        pending_location_ = lexer_.literal_suffix_location();
        pending_range_ = {pending_location_.offset, current_.range.end};
        result.range.end = lexer_.literal_physical_end();
        result.kind = PostKind::array; result.type = type(encoding(s));
        result.width = width(encoding(s)); result.elements = 1;
        sequence_units_.clear(); unit(sequence_units_, 0, result.width);
        result.units = sequence_units_.data();
        ++metrics_.string_sequences;
        metrics_.converted_bytes += result.width;
        if (render_source_) result.source = s.substr(0, lexer_.literal_end());
        ready_ = false; after_operator_ = false; ++metrics_.tokens; return result;
    }
    if (is_string(current_.kind) && !controlling_) { strings(result); after_operator_ = false; }
    else {
        if (render_source_) result.source = s;
        switch (current_.kind) {
        case TokenKind::eof: result.kind = PostKind::eof; break;
        case TokenKind::newline: result.kind = PostKind::newline; break;
        case TokenKind::identifier:
            if (controlling_) {
                result.kind = PostKind::identifier; result.identifier = current_.identifier;
                break;
            }
            // Normal post-tokenization classifies keywords.
            // fall through
        case TokenKind::punctuator:
            if (classify_simple(s, result.simple)) result.kind = PostKind::simple;
            else if (current_.kind == TokenKind::identifier) {
                result.kind = PostKind::identifier; result.identifier = current_.identifier;
            }
            break;
        case TokenKind::number:
            ++metrics_.numbers; convert_number(s, identifiers_, result);
            if (result.kind == PostKind::ud_integer || result.kind == PostKind::ud_floating) {
                numeric_spelling_.assign(s.data(), result.numeric_prefix);
                result.numeric_data = numeric_spelling_.data();
            }
            break;
        case TokenKind::character:
        case TokenKind::ud_character: character(result); break;
        default: break;
        }
        after_operator_ = result.kind == PostKind::simple && result.simple == SimpleKind::KW_OPERATOR;
        ready_ = false;
    }
    ++metrics_.tokens; return result;
}
}
