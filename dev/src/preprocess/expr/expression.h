#pragma once
#include "preprocess/post/cursor.h"

namespace cppgm {
struct PPValue { std::uint64_t bits; bool is_unsigned; };
struct ExpressionMetrics {
    std::size_t lines = 0, nodes = 0, evaluated = 0, max_nodes = 0, max_stack = 0;
    double parse_seconds = 0, evaluate_seconds = 0;
};
// Context and callback belong to the invoking preprocessor. PA3 supplies its
// mock; PA4 can query its macro table by the same canonical identifier ID.
using DefinedQuery = bool (*)(void*, IdentifierId);
class ControllingExpression {
    struct Node {
        SimpleKind op;
        std::size_t a, b, c;
        std::uint64_t bits;
        bool is_unsigned, leaf, unary;
    };
    struct Operator {
        SimpleKind op;
        unsigned precedence;
        std::size_t base;
        bool unary;
    };
    struct Frame { std::size_t node; unsigned state; };
    PostCursor& cursor_;
    IdentifierTable& identifiers_;
    DefinedQuery defined_;
    void* context_;
    bool telemetry_;
    PostToken token_;
    std::vector<Node> nodes_;
    std::vector<std::size_t> values_;
    std::vector<Operator> operators_;
    std::vector<Frame> frames_;
    ExpressionMetrics metrics_;
    IdentifierId true_, defined_id_;
    void advance();
    void reduce();
    void reduce_before(unsigned precedence);
    void leaf(PPValue value);
    PPValue literal() const;
    bool parse();
    PPValue evaluate();
public:
    ControllingExpression(PostCursor& cursor, IdentifierTable& identifiers,
        DefinedQuery defined, void* context = nullptr, bool telemetry = false);
    // Empty lines are skipped. false means EOF; invalid nonempty lines return
    // true with valid=false. Phase-1/2/3 exceptions are deliberately not caught.
    bool next(PPValue& value, bool& valid);
    const ExpressionMetrics& metrics() const { return metrics_; }
};
}
