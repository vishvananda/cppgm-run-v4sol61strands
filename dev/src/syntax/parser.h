#pragma once
#include "syntax/tree.h"
#include "preprocess/post/cursor.h"
#include <deque>
namespace cppgm {
// Only a few grammatical lookahead tokens are retained, never a TU token vector.
class SyntaxParser {
    PostCursor& cursor_;
    IdentifierTable& ids_;
    SyntaxTree& tree_;
    struct InputToken : PostToken { NodeId literal_node = 0, name_node = 0; bool counted = false; };
    std::deque<InputToken> lookahead_;
    using Category = SyntaxCategory;
    std::vector<Category> hints_;
    std::vector<SyntaxScopeId> active_;
    std::uint64_t lookup_serial_ = 0;
    std::uint32_t parameter_order_ = 0;
    IdentifierId final_id_, override_id_, attribute_id_;
    SyntaxScopeId create_scope(SyntaxScopeId, bool namespace_scope=false);
    SyntaxScopeId common_namespace(SyntaxScopeId, SyntaxScopeId) const;
    void import_scope(SyntaxScopeId, SyntaxScopeId);
    void enter(SyntaxScopeId);
    SyntaxBinding lookup(IdentifierId, SyntaxScopeId, bool parents=true, bool qualifier=false, bool namespace_only=false);
    Category hint(IdentifierId);
    void bind(IdentifierId, Category, SyntaxScopeId target=0);
    void prepare_name(unsigned offset=0);
    NodeId namespace_declaration();
    NodeId using_declaration();
    NodeId enum_specifier();
    NodeId class_specifier();
    NodeId class_name(SyntaxKind);
    NodeId special_member(NodeId specs=0, NodeId parsed_name=0);
    NodeId function_body(NodeId, bool ready=false);
    NodeId ctor_initializer();
    bool special_start();
    struct ClassContext { IdentifierId name; SyntaxScopeId scope; };
    std::vector<ClassContext> classes_;
    // The sole retained token representation is for complete-class contexts.
    // Scalar/array literal bytes already live in the TU literal arena.
    struct DeferredToken {
        PostKind kind; SimpleKind simple; IdentifierId identifier;
        SourceRange range; SourceLocation location; NodeId literal;
    };
    enum class DeferredKind : unsigned char { body, equal_initializer, direct_initializer, expression, parameter_tail, member_initializer_tail };
    struct DeferredBody {
        NodeId owner; SyntaxScopeId scope; NodeId siblings = 0; DeferredKind kind = DeferredKind::body;
        std::vector<DeferredToken> tokens;
    };
    std::vector<DeferredBody> deferred_;
    const std::vector<DeferredToken>* deferred_input_ = nullptr;
    std::size_t deferred_position_ = 0;
    void defer_body(NodeId);
    void defer_expression(NodeId, DeferredKind, SimpleKind close, NodeId siblings=0);
    void finish_bodies(std::size_t);
    unsigned delimiter_depth_ = 0;
    std::vector<unsigned> angle_boundaries_;
    unsigned template_depth_ = 0;
    bool member_name_ = false;
    unsigned instantiation_depth_ = 0;
    bool angle_end();
    bool close_angle();
    NodeId template_clause();
    NodeId template_declaration();
    NodeId template_arguments();
    NodeId template_argument();
    NodeId qualified_component(SyntaxKind);
    NodeId qualified_raw(SyntaxKind, bool namespace_only=false);
    std::size_t tokens_=0, queries_=0, max_lookahead_=0;
    InputToken take();
    const PostToken& peek(unsigned offset=0);
    bool at(SimpleKind, unsigned offset=0);
    bool eat(SimpleKind);
    bool require(SimpleKind);
    NodeId error(const char*);
    const char* expected_ = nullptr;
    SourceLocation error_location_;
    void enter();
    void leave();
    Category category(IdentifierId);
    bool type_start(unsigned offset=0);
    bool specifier(SimpleKind) const;
    bool builtin(SimpleKind) const;
    NodeId leaf(SyntaxKind, const PostToken&);
    NodeId raw(SyntaxKind, SimpleKind);
    NodeId name(SyntaxKind);
    NodeId specs(bool type=false, bool force=false, NodeId result=0, bool has_type=false, bool allow_special=false);
    NodeId declarator(bool abstract=false, bool allow_name=true, bool allocation=false);
    NodeId parameters();
    NodeId parameter_region(bool mixed=false);
    void parameter_entries(NodeId, bool mixed, bool tail=false);
    NodeId parameter_expression(NodeId);
    NodeId type_id(bool allocation=false, bool force=false);
    NodeId initializer(bool equal=false);
    NodeId list(SyntaxKind, SimpleKind close);
    NodeId expression(int minimum=1);
    NodeId expression_tail(NodeId, int minimum=1);
    NodeId unary();
    NodeId unary_atom();
    NodeId primary();
    NodeId allocation();
    NodeId lambda();
    NodeId attributes(NodeId owner=0);
    NodeId postfix(NodeId);
    NodeId declaration();
    NodeId declaration_impl();
    NodeId declaration_tail(NodeId, bool);
    NodeId compound();
    NodeId statement();
    NodeId scoped_statement();
    NodeId condition();
    NodeId for_statement();
    NodeId ambiguous_statement();
    NodeId suffixes(NodeId, bool allocation=false);
    NodeId qualified(SyntaxKind, bool namespace_only=false);
    NodeId try_statement(NodeId result=0);
    IdentifierId declared_name(NodeId) const;
    bool function_declarator(NodeId) const;
public:
    SyntaxParser(PostCursor& cursor, IdentifierTable& ids, SyntaxTree& tree)
        : cursor_(cursor), ids_(ids), tree_(tree), final_id_(ids.intern("final")), override_id_(ids.intern("override")), attribute_id_(ids.intern("__attribute__")) { tree_.scopes.emplace_back(); enter(); }
    NodeId parse();
    const char* expected() const { return expected_; }
    SourceLocation error_location() const { return error_location_; }
    std::size_t tokens() const { return tokens_; }
    std::size_t queries() const { return queries_; }
    std::size_t max_lookahead() const { return max_lookahead_; }
};
}
