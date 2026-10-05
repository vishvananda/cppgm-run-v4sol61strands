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
    struct InputToken : PostToken { NodeId literal_node = 0, name_node = 0; };
    std::deque<InputToken> lookahead_;
    using Category = SyntaxCategory;
    std::vector<Category> hints_;
    std::vector<SyntaxScopeId> active_;
    std::uint64_t lookup_serial_ = 0;
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
    NodeId qualified_component(SyntaxKind);
    NodeId qualified_raw(SyntaxKind, bool namespace_only=false);
    std::size_t tokens_=0, queries_=0, max_lookahead_=0;
    InputToken take();
    const PostToken& peek(unsigned offset=0);
    bool at(SimpleKind, unsigned offset=0);
    bool eat(SimpleKind);
    void require(SimpleKind);
    [[noreturn]] void error(const char*);
    void enter();
    void leave();
    Category category(IdentifierId);
    bool type_start(unsigned offset=0);
    bool specifier(SimpleKind) const;
    bool builtin(SimpleKind) const;
    NodeId leaf(SyntaxKind, const PostToken&);
    NodeId raw(SyntaxKind, SimpleKind);
    NodeId name(SyntaxKind);
    NodeId specs(bool type=false, bool force=false, NodeId result=0, bool has_type=false);
    NodeId declarator(bool abstract=false, bool allow_name=true, bool allocation=false);
    NodeId parameters();
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
    void attributes();
    NodeId postfix(NodeId);
    NodeId declaration();
    NodeId declaration_tail(NodeId, bool);
    NodeId compound();
    NodeId statement();
    NodeId scoped_statement();
    NodeId condition();
    NodeId for_statement();
    NodeId ambiguous_statement();
    NodeId suffixes(NodeId, bool allocation=false);
    NodeId qualified(SyntaxKind, bool namespace_only=false);
    NodeId try_statement();
    IdentifierId declared_name(NodeId) const;
    bool function_declarator(NodeId) const;
public:
    SyntaxParser(PostCursor& cursor, IdentifierTable& ids, SyntaxTree& tree)
        : cursor_(cursor), ids_(ids), tree_(tree) { tree_.scopes.emplace_back(); enter(); }
    NodeId parse();
    std::size_t tokens() const { return tokens_; }
    std::size_t queries() const { return queries_; }
    std::size_t max_lookahead() const { return max_lookahead_; }
};
}
