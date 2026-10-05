#pragma once
#include "preprocess/post/types.h"

namespace cppgm {
struct PostMetrics {
    std::size_t tokens = 0, numbers = 0, literal_elements = 0, string_sequences = 0;
    std::size_t converted_bytes = 0, max_sequence_elements = 0;
};
// Owns one PP lookahead; literal values come from lexing, not a spelling reparse.
class PostCursor {
    PPSource& lexer_;
    IdentifierTable& identifiers_;
    bool render_source_, controlling_, ready_ = false, after_operator_ = false;
    Token current_;
    IdentifierId pending_identifier_ = 0;
    SourceRange pending_range_ = {0, 0};
    SourceLocation pending_location_ = {0, 0, 0};
    PostMetrics metrics_;
    std::vector<LiteralElement> sequence_elements_;
    std::vector<unsigned char> sequence_units_;
    std::string numeric_spelling_;
    void advance();
    void strings(PostToken& result);
    void character(PostToken& result);
public:
    PostCursor(PPSource& lexer, IdentifierTable& identifiers, bool render_source = false, bool controlling = false)
        : lexer_(lexer), identifiers_(identifiers), render_source_(render_source), controlling_(controlling) {}
    PostToken next();
    const PostMetrics& metrics() const { return metrics_; }
};
}
