#include "preprocess/expr/expression.h"
#include <algorithm>
#include <chrono>

namespace cppgm {
namespace {
struct InvalidExpression {};
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
std::uint64_t binary(SimpleKind op, PPValue a, PPValue b) {
    const bool u = a.is_unsigned || b.is_unsigned;
    const std::uint64_t x = a.bits, y = b.bits;
    // Unsigned storage gives defined modulo arithmetic even for course-defined
    // two's-complement signed results; host signed overflow is never invoked.
    const bool less = u ? x < y : (x ^ sign) < (y ^ sign);
    switch (op) {
    case SimpleKind::OP_PLUS: return x + y;
    case SimpleKind::OP_MINUS: return x - y;
    case SimpleKind::OP_STAR: return x * y;
    case SimpleKind::OP_DIV: case SimpleKind::OP_MOD: {
        if (!y || (!u && x == sign && y == ~std::uint64_t(0))) throw InvalidExpression();
        if (u) return op == SimpleKind::OP_DIV ? x/y : x%y;
        const bool nx = (x & sign) != 0, ny = (y & sign) != 0;
        const auto ax = nx ? -x : x, ay = ny ? -y : y;
        const auto result = op == SimpleKind::OP_DIV ? ax/ay : ax%ay;
        return (op == SimpleKind::OP_DIV ? nx != ny : nx) ? -result : result;
    }
    case SimpleKind::OP_LSHIFT: case SimpleKind::OP_RSHIFT:
        if (y >= 64) throw InvalidExpression();
        if (op == SimpleKind::OP_LSHIFT) return x << y;
        if (y && !a.is_unsigned && (x & sign)) return (x >> y) | (~std::uint64_t(0) << (64-y));
        return x >> y;
    case SimpleKind::OP_LT: return less;
    case SimpleKind::OP_GT: return x != y && !less;
    case SimpleKind::OP_LE: return x == y || less;
    case SimpleKind::OP_GE: return !less;
    case SimpleKind::OP_EQ: return x == y;
    case SimpleKind::OP_NE: return x != y;
    case SimpleKind::OP_AMP: return x & y;
    case SimpleKind::OP_XOR: return x ^ y;
    case SimpleKind::OP_BOR: return x | y;
    case SimpleKind::OP_LAND: return x && y;
    case SimpleKind::OP_LOR: return x || y;
    default: throw InvalidExpression();
    }
}
}
ControllingExpression::ControllingExpression(PostCursor& cursor, IdentifierTable& identifiers,
    DefinedQuery defined, void* context, bool telemetry)
    : cursor_(cursor), identifiers_(identifiers), defined_(defined), context_(context),
      telemetry_(telemetry), true_(identifiers.intern("true")),
      defined_id_(identifiers.intern("defined")) {
    advance();
}
void ControllingExpression::advance() { token_ = cursor_.next(); }
void ControllingExpression::leaf(PPValue value) {
    nodes_.push_back({SimpleKind::OP_PLUS, 0, 0, 0, value.bits, value.is_unsigned, true, false});
    values_.push_back(nodes_.size()-1);
}
PPValue ControllingExpression::literal() const {
    if (token_.kind != PostKind::scalar || token_.type > FundamentalType::FT_BOOL)
        throw InvalidExpression();
    const bool u = unsigned_type(token_.type);
    std::uint64_t bits = 0;
    for (std::size_t i = 0; i < token_.width; ++i)
        bits |= std::uint64_t(token_.scalar[i]) << (8*i);
    if (!u && token_.width < 8 && (bits & (std::uint64_t(1) << (8*token_.width-1))))
        bits |= ~std::uint64_t(0) << (8*token_.width);
    return {bits, u};
}
void ControllingExpression::reduce() {
    if (operators_.empty()) throw InvalidExpression();
    const Operator o = operators_.back(); operators_.pop_back();
    const std::size_t count = o.unary ? 1 : o.op == SimpleKind::OP_COLON ? 3 : 2;
    if (o.op == SimpleKind::OP_LPAREN || o.op == SimpleKind::OP_QMARK ||
        values_.size() != o.base + count) throw InvalidExpression();
    const auto a = values_[o.base];
    const auto b = count > 1 ? values_[o.base+1] : 0;
    const auto c = count > 2 ? values_[o.base+2] : 0;
    bool u = nodes_[a].is_unsigned;
    if (o.op == SimpleKind::OP_COLON) u = nodes_[b].is_unsigned || nodes_[c].is_unsigned;
    else if (o.unary) { if (o.op == SimpleKind::OP_LNOT) u = false; }
    else if (o.precedence == 2 || o.precedence == 3 || o.precedence == 7 || o.precedence == 8) u = false;
    else if (o.precedence != 9) u = u || nodes_[b].is_unsigned;
    values_.resize(o.base);
    nodes_.push_back({o.op, a, b, c, 0, u, false, o.unary});
    values_.push_back(nodes_.size()-1);
}
void ControllingExpression::reduce_before(unsigned p) {
    while (!operators_.empty() && operators_.back().precedence >= p) reduce();
}
bool ControllingExpression::parse() {
    bool operand = true;
    while (token_.kind != PostKind::newline && token_.kind != PostKind::eof) {
        if (operand) {
            if (token_.kind == PostKind::simple) {
                const auto op = token_.simple;
                if (op == SimpleKind::OP_LPAREN) operators_.push_back({op, 0, values_.size(), false});
                else if (op == SimpleKind::OP_PLUS || op == SimpleKind::OP_MINUS ||
                    op == SimpleKind::OP_LNOT || op == SimpleKind::OP_COMPL)
                    operators_.push_back({op, 12, values_.size(), true});
                else throw InvalidExpression();
                advance(); continue;
            }
            if (token_.kind == PostKind::identifier) {
                const auto id = token_.identifier;
                advance();
                if (id == defined_id_) {
                    const bool paren = token_.kind == PostKind::simple && token_.simple == SimpleKind::OP_LPAREN;
                    if (paren) advance();
                    if (token_.kind != PostKind::identifier) throw InvalidExpression();
                    const auto name = token_.identifier;
                    advance();
                    if (paren) {
                        if (token_.kind != PostKind::simple || token_.simple != SimpleKind::OP_RPAREN)
                            throw InvalidExpression();
                        advance();
                    }
                    leaf({defined_ && defined_(context_, name) ? 1u : 0u, false});
                } else leaf({id == true_ ? 1u : 0u, false});
            } else { leaf(literal()); advance(); }
            operand = false;
        } else {
            if (token_.kind != PostKind::simple) throw InvalidExpression();
            const auto op = token_.simple;
            if (op == SimpleKind::OP_RPAREN) {
                while (!operators_.empty() && operators_.back().op != SimpleKind::OP_LPAREN) reduce();
                if (operators_.empty() || values_.size() != operators_.back().base+1) throw InvalidExpression();
                operators_.pop_back();
            } else if (op == SimpleKind::OP_QMARK) {
                reduce_before(2);
                operators_.push_back({op, 1, values_.size()-1, false});
                operand = true;
            } else if (op == SimpleKind::OP_COLON) {
                while (!operators_.empty() && operators_.back().op != SimpleKind::OP_QMARK) reduce();
                if (operators_.empty() || values_.size() != operators_.back().base+2) throw InvalidExpression();
                operators_.back().op = SimpleKind::OP_COLON;
                operand = true;
            } else {
                const unsigned p = precedence(op);
                if (!p) throw InvalidExpression();
                reduce_before(p);
                operators_.push_back({op, p, values_.size()-1, false});
                operand = true;
            }
            advance();
        }
    }
    if (operand) throw InvalidExpression();
    while (!operators_.empty()) reduce();
    if (values_.size() != 1) throw InvalidExpression();
    return true;
}
PPValue ControllingExpression::evaluate() {
    frames_.push_back({values_[0], 0});
    while (!frames_.empty()) {
        metrics_.max_stack = std::max(metrics_.max_stack, frames_.size());
        auto& frame = frames_.back();
        auto& n = nodes_[frame.node];
        if (n.leaf) { ++metrics_.evaluated; frames_.pop_back(); continue; }
        if (!frame.state) { frame.state = 1; frames_.push_back({n.a, 0}); continue; }
        const auto x = nodes_[n.a].bits;
        if (n.op == SimpleKind::OP_COLON) {
            const auto chosen = x ? n.b : n.c;
            if (frame.state == 1) { frame.state = 2; frames_.push_back({chosen, 0}); continue; }
            n.bits = nodes_[chosen].bits;
        } else if (n.op == SimpleKind::OP_LAND && !x) n.bits = 0;
        else if (n.op == SimpleKind::OP_LOR && x) n.bits = 1;
        else if (n.op == SimpleKind::OP_LNOT) n.bits = !x;
        else if (n.op == SimpleKind::OP_COMPL) n.bits = ~x;
        else if (n.unary) n.bits = n.op == SimpleKind::OP_PLUS ? x : -x;
        else {
            if (frame.state == 1) { frame.state = 2; frames_.push_back({n.b, 0}); continue; }
            n.bits = binary(n.op, {x, nodes_[n.a].is_unsigned}, {nodes_[n.b].bits, nodes_[n.b].is_unsigned});
        }
        ++metrics_.evaluated; frames_.pop_back();
    }
    const auto& root = nodes_[values_[0]];
    return {root.bits, root.is_unsigned};
}
bool ControllingExpression::next(PPValue& value, bool& valid) {
    while (token_.kind == PostKind::newline) advance();
    if (token_.kind == PostKind::eof) return false;
    nodes_.clear(); values_.clear(); operators_.clear(); frames_.clear();
    ++metrics_.lines;
    Clock::time_point start;
    if (telemetry_) start = Clock::now();
    valid = false;
    try { parse(); valid = true; } catch (const InvalidExpression&) {}
    if (telemetry_) metrics_.parse_seconds += std::chrono::duration<double>(Clock::now()-start).count();
    if (!valid) {
        // Preserve lexical errors even in the remainder of an invalid line.
        while (token_.kind != PostKind::newline && token_.kind != PostKind::eof) advance();
    } else {
        if (telemetry_) start = Clock::now();
        try { value = evaluate(); } catch (const InvalidExpression&) { valid = false; }
        if (telemetry_) metrics_.evaluate_seconds += std::chrono::duration<double>(Clock::now()-start).count();
    }
    metrics_.nodes += nodes_.size();
    metrics_.max_nodes = std::max(metrics_.max_nodes, nodes_.size());
    return true;
}
}
