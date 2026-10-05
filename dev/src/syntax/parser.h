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
    std::deque<PostToken> lookahead_;
    enum class Category : unsigned char { unknown, value, type, templ, space };
    std::vector<Category> bindings_, hints_;
    struct Change { IdentifierId id; Category previous; };
    std::vector<Change> changes_;
    std::vector<std::size_t> scopes_;
    std::size_t tokens_=0, queries_=0, max_lookahead_=0;
    PostToken take();
    const PostToken& peek(unsigned offset=0);
    bool at(SimpleKind, unsigned offset=0);
    bool eat(SimpleKind);
    void require(SimpleKind);
    [[noreturn]] void error(const char*);
    void enter();
    void leave();
    void bind(IdentifierId, Category);
    Category category(IdentifierId);
    bool type_start(unsigned offset=0);
    bool specifier(SimpleKind) const;
    bool builtin(SimpleKind) const;
    NodeId leaf(SyntaxKind, const PostToken&);
    NodeId raw(SyntaxKind, SimpleKind);
    NodeId name(SyntaxKind);
    NodeId specs(bool type=false);
    NodeId declarator(bool abstract=false, bool allow_name=true);
    NodeId parameters();
    NodeId type_id();
    NodeId initializer(bool equal=false);
    NodeId list(SyntaxKind, SimpleKind close);
    NodeId expression(int minimum=1);
    NodeId unary();
    NodeId primary();
    NodeId allocation();
    NodeId lambda();
    void attributes();
    NodeId postfix(NodeId);
    NodeId declaration();
    NodeId compound();
    NodeId statement();
    NodeId condition();
    NodeId for_statement();
    NodeId try_statement();
    IdentifierId declared_name(NodeId) const;
    bool function_declarator(NodeId) const;
public:
    SyntaxParser(PostCursor& cursor, IdentifierTable& ids, SyntaxTree& tree)
        : cursor_(cursor), ids_(ids), tree_(tree) { enter(); }
    NodeId parse();
    std::size_t tokens() const { return tokens_; }
    std::size_t queries() const { return queries_; }
    std::size_t max_lookahead() const { return max_lookahead_; }
};
}
