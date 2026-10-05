#pragma once
#include "preprocess/post/cursor.h"

namespace cppgm {
struct PPValue { std::uint64_t bits; bool is_unsigned; };
struct ExpressionMetrics {
    std::size_t lines = 0, nodes = 0, evaluated = 0, max_nodes = 0, max_stack = 0;
    std::size_t reductions = 0, deferred_errors = 0;
    double parse_seconds = 0, evaluate_seconds = 0;
};
// Context and callback belong to the invoking preprocessor. PA3 supplies its
// mock; PA4 can query its macro table by the same canonical identifier ID.
using DefinedQuery = bool (*)(void*, IdentifierId);
class ControllingExpression {
    // Every PA3 terminal is already a constant after token conversion/defined.
    // Keep its typed value, source range and deferred domain error, not a dead
    // tree for a second traversal. Lazy operators select which error survives.
    struct Fact {
        SourceRange range;
        PPValue value;
        bool valid;
    };
    struct Operator {
        SimpleKind op;
        unsigned precedence;
        std::uint32_t base;
        bool unary;
        std::size_t begin;
    };
    PostCursor& cursor_;
    DefinedQuery defined_;
    void* context_;
    bool telemetry_;
    PostToken token_;
    std::vector<Fact> facts_;
    std::vector<Operator> operators_;
    ExpressionMetrics metrics_;
    IdentifierId true_, defined_id_;
    void advance();
    void push_operator(SimpleKind op, unsigned precedence, bool unary);
    bool reduce();
    bool reduce_before(unsigned precedence);
    void leaf(PPValue value, SourceRange range);
    bool literal(PPValue& value) const;
    bool parse();
    bool result(PPValue& value) const;
public:
    ControllingExpression(PostCursor& cursor, IdentifierTable& identifiers,
        DefinedQuery defined, void* context = nullptr, bool telemetry = false);
    // Empty lines are skipped. false means EOF; invalid nonempty lines return
    // true with valid=false. Phase-1/2/3 exceptions are deliberately not caught.
    bool next(PPValue& value, bool& valid);
    const ExpressionMetrics& metrics() const { return metrics_; }
};
}
