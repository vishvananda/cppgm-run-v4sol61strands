#pragma once
#include "preprocess/lex/lexer.h"
#include <functional>

namespace cppgm {
// These buffers are retained only for definitions, operands, arguments and
// pending rescan. Ordinary source text is pulled one token at a time.
struct PPItem {
    Token token = {TokenKind::eof, {0,0}, {0,0,0}, 0};
    std::string text;
    std::vector<LiteralElement> elements;
    std::size_t literal_end = 0, physical_end = 0;
    SourceLocation suffix_location = {0,0,0};
    std::uint32_t paint = 0, file = 0;
    std::size_t line = 1;
    bool space = false, blocked = false;
};
struct PreprocessorMetrics {
    std::size_t source_bytes = 0, source_tokens = 0, expanded = 0;
    std::size_t invocations = 0, argument_tokens = 0, paste_tokens = 0;
    std::size_t lookups = 0, max_pending = 0, files = 0;
    std::size_t paint_nodes = 0, paint_queries = 0, paint_cache_hits = 0;
    std::size_t indexed_tokens = 0, argument_spans = 0, max_tasks = 0;
};
// TU-local ownership, indexed definitions and persistent nesting paint.
// No output dump is ever reparsed by the next production phase.
class MacroEngine {
public:
    using Pull = std::function<bool(PPItem&)>;
    using Builtin = std::function<bool(PPItem&)>;
    class Rescan {
        struct State;
        std::unique_ptr<State> state_;
        friend class MacroEngine;
    public:
        Rescan();
        ~Rescan();
        Rescan(const Rescan&) = delete;
        Rescan& operator=(const Rescan&) = delete;
    };
private:
    struct Replacement { PPItem item; int parameter = -1; };
    struct Macro {
        IdentifierId name = 0;
        bool function = false, variadic = false;
        std::vector<IdentifierId> parameters;
        std::vector<Replacement> replacement;
        std::vector<std::uint32_t> prescan_parameters;
    };
    struct Paint { std::uint32_t child[2]; };
    IdentifierTable& identifiers_;
    PreprocessorMetrics& metrics_;
    std::vector<std::uint32_t> bindings_;
    std::vector<Macro> definitions_;
    std::vector<std::uint32_t> free_definitions_;
    std::vector<Paint> paints_;
    // Cache key is (immutable paint root, interned macro ID); extensions stay
    // valid throughout this TU, independent of macro redefinition.
    struct Extension { std::uint32_t paint, macro, result; };
    std::vector<Extension> extensions_;
    std::vector<std::uint32_t> extension_slots_;
    void grow_extensions();
    std::vector<int> parameter_slots_;
    IdentifierId va_;
    bool painted(std::uint32_t paint, IdentifierId name) const;
    std::uint32_t add_paint(std::uint32_t paint, IdentifierId name);
    std::uint32_t merge_paint(std::uint32_t a, std::uint32_t b, unsigned bit = 0);
    std::uint32_t intersect_paint(std::uint32_t a, std::uint32_t b, unsigned bit = 0);
    std::uint32_t insert_paint(std::uint32_t p, IdentifierId id, unsigned bit);
    struct Sequence;
    struct Span {
        std::shared_ptr<const Sequence> sequence;
        std::size_t begin, end;
    };
    std::shared_ptr<const Sequence> sequence(std::vector<PPItem>);
    std::vector<Span> arguments(const Macro&, const std::shared_ptr<const Sequence>&, std::size_t open, std::size_t close);
    void substitute(const Macro&, const PPItem&, std::uint32_t, const std::vector<Span>&, const std::vector<std::vector<PPItem>>&, std::vector<PPItem>&);
public:
    explicit MacroEngine(IdentifierTable&, PreprocessorMetrics&);
    bool defined(IdentifierId) const;
    void define(const std::vector<PPItem>&);
    void undefine(const std::vector<PPItem>&);
    // Rescan owns bounded chunks and explicit prescan tasks, not a C++ call stack.
    // Pull false marks text-sequence end; definitions remain stable until drained.
    bool next(PPItem&, Rescan&, const Pull&, const Builtin&);
    std::vector<PPItem> expand(const std::vector<PPItem>&, const Builtin&);
    std::vector<PPItem> expand_owned(std::vector<PPItem>, const Builtin&);
    PPItem synthetic(const std::string&, const PPItem&);
};

class Preprocessor : public PPSource {
    struct Impl;
    std::unique_ptr<Impl> impl_;
    PPItem current_;
public:
    Preprocessor(IdentifierTable&, const std::string& source, const std::string& date, const std::string& time);
    ~Preprocessor();
    Token next() override;
    const std::string& spelling() const override { return current_.text; }
    const std::vector<LiteralElement>& literal_elements() const override { return current_.elements; }
    std::size_t literal_end() const override { return current_.literal_end; }
    std::size_t literal_physical_end() const override { return current_.physical_end; }
    SourceLocation literal_suffix_location() const override { return current_.suffix_location; }
    const PreprocessorMetrics& metrics() const;
    const std::string& file_name(std::uint32_t) const;
    const SourceBuffer& source_buffer(std::uint32_t) const;
};
PPItem capture(Lexer&, Token);
bool punctuation(const PPItem&, const char*);
}
