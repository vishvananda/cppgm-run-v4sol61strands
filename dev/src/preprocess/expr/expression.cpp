#include "preprocess/expr/expression.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>

namespace cppgm {
namespace {
using Clock = std::chrono::steady_clock;
constexpr std::uint64_t sign = std::uint64_t(1) << 63;
unsigned precedence(SimpleKind op) {
    switch (op) {
    case SimpleKind::OP_LOR: return 2;
    case SimpleKind::OP_LAND: return 3;
    case SimpleKind::OP_BOR: return 4;
    case SimpleKind::OP_XOR: return 5;
    case SimpleKind::OP_AMP: return 6;
    case SimpleKind::OP_EQ: case SimpleKind::OP_NE: return 7;
    case SimpleKind::OP_LT: case SimpleKind::OP_GT:
    case SimpleKind::OP_LE: case SimpleKind::OP_GE: return 8;
    case SimpleKind::OP_LSHIFT: case SimpleKind::OP_RSHIFT: return 9;
    case SimpleKind::OP_PLUS: case SimpleKind::OP_MINUS: return 10;
    case SimpleKind::OP_STAR: case SimpleKind::OP_DIV: case SimpleKind::OP_MOD: return 11;
    default: return 0;
    }
}
bool unsigned_type(FundamentalType type) {
    switch (type) {
    case FundamentalType::FT_UNSIGNED_CHAR: case FundamentalType::FT_UNSIGNED_SHORT_INT:
    case FundamentalType::FT_UNSIGNED_INT: case FundamentalType::FT_UNSIGNED_LONG_INT:
    case FundamentalType::FT_UNSIGNED_LONG_LONG_INT:
    case FundamentalType::FT_CHAR16_T: case FundamentalType::FT_CHAR32_T: return true;
    default: return false;
    }
}
bool binary(SimpleKind op, PPValue a, PPValue b, std::uint64_t& result) {
    const bool u = a.is_unsigned || b.is_unsigned;
    const std::uint64_t x = a.bits, y = b.bits;
    // Unsigned storage avoids host signed-overflow UB. Portable comparisons
    // exclude source-level signed overflow; course shift/domain rules are
    // checked explicitly below.
    const bool less = u ? x < y : (x ^ sign) < (y ^ sign);
    switch (op) {
    case SimpleKind::OP_PLUS: { result = x + y; return true; }
    case SimpleKind::OP_MINUS: { result = x - y; return true; }
    case SimpleKind::OP_STAR: { result = x * y; return true; }
    case SimpleKind::OP_DIV: case SimpleKind::OP_MOD: {
        if (!y || (!u && x == sign && y == ~std::uint64_t(0))) return false;
        if (u) { result = op == SimpleKind::OP_DIV ? x/y : x%y; return true; }
        const bool nx = (x & sign) != 0, ny = (y & sign) != 0;
        const auto ax = nx ? -x : x, ay = ny ? -y : y;
        const auto magnitude = op == SimpleKind::OP_DIV ? ax/ay : ax%ay;
        { result = (op == SimpleKind::OP_DIV ? nx != ny : nx) ? -magnitude : magnitude; return true; }
    }
    case SimpleKind::OP_LSHIFT: case SimpleKind::OP_RSHIFT:
        if (y >= 64) return false;
        if (op == SimpleKind::OP_LSHIFT) { result = x << y; return true; }
        if (y && !a.is_unsigned && (x & sign)) { result = (x >> y) | (~std::uint64_t(0) << (64-y)); return true; }
        { result = x >> y; return true; }
    case SimpleKind::OP_LT: { result = less; return true; }
    case SimpleKind::OP_GT: { result = x != y && !less; return true; }
    case SimpleKind::OP_LE: { result = x == y || less; return true; }
    case SimpleKind::OP_GE: { result = !less; return true; }
    case SimpleKind::OP_EQ: { result = x == y; return true; }
    case SimpleKind::OP_NE: { result = x != y; return true; }
    case SimpleKind::OP_AMP: { result = x & y; return true; }
    case SimpleKind::OP_XOR: { result = x ^ y; return true; }
    case SimpleKind::OP_BOR: { result = x | y; return true; }
    case SimpleKind::OP_LAND: { result = x && y; return true; }
    case SimpleKind::OP_LOR: { result = x || y; return true; }
    default: return false;
    }
}
}
ControllingExpression::ControllingExpression(PostCursor& cursor, IdentifierTable& identifiers,
    DefinedQuery defined, void* context, bool telemetry)
    : cursor_(cursor), defined_(defined), context_(context),
      telemetry_(telemetry), true_(identifiers.intern("true")),
      defined_id_(identifiers.intern("defined")) {
    advance();
}
void ControllingExpression::advance() { token_ = cursor_.next(); }
inline void ControllingExpression::push_operator(SimpleKind op, unsigned p, bool unary) {
    const auto base = facts_.size() - (p && !unary ? 1 : 0);
    operators_.push_back({op, p, static_cast<std::uint32_t>(base), unary, token_.range.begin});
    metrics_.max_stack = std::max(metrics_.max_stack, operators_.size());
}
void ControllingExpression::leaf(PPValue value, SourceRange range) {
    if (facts_.size() == std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("expression fact stack exhausted");
    facts_.push_back({range, value, true});
    ++metrics_.nodes;
    metrics_.max_nodes = std::max(metrics_.max_nodes, facts_.size());
}
bool ControllingExpression::literal(PPValue& value) const {
    if (token_.kind != PostKind::scalar || token_.type > FundamentalType::FT_BOOL)
        return false;
    const bool u = unsigned_type(token_.type);
    std::uint64_t bits = 0;
    for (std::size_t i = 0; i < token_.width; ++i)
        bits |= std::uint64_t(token_.scalar[i]) << (8*i);
    if (!u && token_.width < 8 && (bits & (std::uint64_t(1) << (8*token_.width-1))))
        bits |= ~std::uint64_t(0) << (8*token_.width);
    value = {bits, u};
    return true;
}
bool ControllingExpression::reduce() {
    if (operators_.empty()) return false;
    const Operator o = operators_.back(); operators_.pop_back();
    const std::size_t count = o.unary ? 1 : o.op == SimpleKind::OP_COLON ? 3 : 2;
    if (o.op == SimpleKind::OP_LPAREN || o.op == SimpleKind::OP_QMARK ||
        facts_.size() != o.base + count) return false;
    Fact& a = facts_[o.base];
    // Grammar/literal errors reject parse(), including unchosen operands.
    // Arithmetic domain errors remain typed facts until lazy selection. Never
    // execute a trapping operation: binary checks its domain before computing.
    bool valid = a.valid;
    PPValue value = a.value;
    if (o.unary) {
        if (o.op == SimpleKind::OP_LNOT) value = {!value.bits, false};
        else if (o.op == SimpleKind::OP_COMPL) value.bits = ~value.bits;
        else if (o.op == SimpleKind::OP_MINUS) value.bits = -value.bits;
    } else {
        const Fact& b = facts_[o.base+1];
        if (o.op == SimpleKind::OP_COLON) {
            const Fact& c = facts_[o.base+2];
            const Fact& chosen = value.bits ? b : c;
            valid = valid && chosen.valid;
            value = {chosen.value.bits, b.value.is_unsigned || c.value.is_unsigned};
        } else if (o.op == SimpleKind::OP_LAND || o.op == SimpleKind::OP_LOR) {
            const bool choose_b = o.op == SimpleKind::OP_LAND ? value.bits != 0 : value.bits == 0;
            valid = valid && (!choose_b || b.valid);
            value = {choose_b ? std::uint64_t(b.value.bits != 0) : std::uint64_t(value.bits != 0), false};
        } else {
            valid = valid && b.valid;
            if (valid) valid = binary(o.op, value, b.value, value.bits);
            if (o.precedence == 7 || o.precedence == 8) value.is_unsigned = false;
            else if (o.precedence != 9) value.is_unsigned |= b.value.is_unsigned;
        }
    }
    a.range = {o.unary ? o.begin : a.range.begin, facts_.back().range.end};
    a.value = value; a.valid = valid;
    facts_.resize(o.base+1);
    ++metrics_.nodes; ++metrics_.reductions;
    if (valid) ++metrics_.evaluated; else ++metrics_.deferred_errors;
    return true;
}
bool ControllingExpression::reduce_before(unsigned p) {
    while (!operators_.empty() && operators_.back().precedence >= p)
        if (!reduce()) return false;
    return true;
}
bool ControllingExpression::parse() {
    bool operand = true;
    while (token_.kind != PostKind::newline && token_.kind != PostKind::eof) {
        if (operand) {
            if (token_.kind == PostKind::simple) {
                const auto op = token_.simple;
                if (op == SimpleKind::OP_LPAREN) push_operator(op, 0, false);
                else if (op == SimpleKind::OP_PLUS || op == SimpleKind::OP_MINUS ||
                    op == SimpleKind::OP_LNOT || op == SimpleKind::OP_COMPL)
                    push_operator(op, 12, true);
                else return false;
                advance(); continue;
            }
            if (token_.kind == PostKind::identifier) {
                const auto id = token_.identifier;
                SourceRange range = token_.range;
                advance();
                if (id == defined_id_) {
                    const bool paren = token_.kind == PostKind::simple && token_.simple == SimpleKind::OP_LPAREN;
                    if (paren) advance();
                    if (token_.kind != PostKind::identifier) return false;
                    const auto name = token_.identifier;
                    range.end = token_.range.end;
                    advance();
                    if (paren) {
                        if (token_.kind != PostKind::simple || token_.simple != SimpleKind::OP_RPAREN)
                            return false;
                        range.end = token_.range.end;
                        advance();
                    }
                    leaf({defined_ && defined_(context_, name) ? 1u : 0u, false}, range);
                } else leaf({id == true_ ? 1u : 0u, false}, range);
            } else {
                PPValue value;
                if (!literal(value)) return false;
                leaf(value, token_.range); advance();
            }
            operand = false;
        } else {
            if (token_.kind != PostKind::simple) return false;
            const auto op = token_.simple;
            if (op == SimpleKind::OP_RPAREN) {
                while (!operators_.empty() && operators_.back().op != SimpleKind::OP_LPAREN)
                    if (!reduce()) return false;
                if (operators_.empty() || facts_.size() != operators_.back().base+1) return false;
                facts_.back().range = {operators_.back().begin, token_.range.end};
                operators_.pop_back();
            } else if (op == SimpleKind::OP_QMARK) {
                if (!reduce_before(2)) return false;
                push_operator(op, 1, false);
                operand = true;
            } else if (op == SimpleKind::OP_COLON) {
                while (!operators_.empty() && operators_.back().op != SimpleKind::OP_QMARK)
                    if (!reduce()) return false;
                if (operators_.empty() || facts_.size() != operators_.back().base+2) return false;
                operators_.back().op = SimpleKind::OP_COLON;
                operand = true;
            } else {
                const unsigned p = precedence(op);
                if (!p) return false;
                if (!reduce_before(p)) return false;
                push_operator(op, p, false);
                operand = true;
            }
            advance();
        }
    }
    if (operand) return false;
    while (!operators_.empty()) if (!reduce()) return false;
    return facts_.size() == 1;
}
bool ControllingExpression::result(PPValue& value) const {
    value = facts_[0].value;
    return facts_[0].valid;
}
bool ControllingExpression::next(PPValue& value, bool& valid) {
    while (token_.kind == PostKind::newline) advance();
    if (token_.kind == PostKind::eof) return false;
    facts_.clear(); operators_.clear();
    ++metrics_.lines;
    Clock::time_point start;
    if (telemetry_) start = Clock::now();
    valid = parse();
    if (telemetry_) metrics_.parse_seconds += std::chrono::duration<double>(Clock::now()-start).count();
    if (!valid) {
        // Preserve lexical errors even in the remainder of an invalid line.
        while (token_.kind != PostKind::newline && token_.kind != PostKind::eof) advance();
    } else {
        if (telemetry_) start = Clock::now();
        valid = result(value);
        if (telemetry_) metrics_.evaluate_seconds += std::chrono::duration<double>(Clock::now()-start).count();
    }
    return true;
}
}
