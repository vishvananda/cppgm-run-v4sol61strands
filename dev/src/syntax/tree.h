#pragma once
#include "preprocess/post/types.h"
#include <ostream>
#include <unordered_map>
#include <unordered_set>

namespace cppgm {
using NodeId = std::uint32_t;
using SyntaxScopeId = std::uint32_t;
enum class SyntaxCategory : unsigned char { unknown, value, type, templ, space };
struct SyntaxBinding {
    SyntaxCategory category = SyntaxCategory::unknown;
    SyntaxScopeId target = 0;
    // Qualifier lookup ignores ordinary values (C++11 3.4.3).
    SyntaxScopeId qualifier = 0;
    SyntaxCategory qualifier_category = SyntaxCategory::unknown;
    SyntaxBinding() = default;
    SyntaxBinding(SyntaxCategory c, SyntaxScopeId t, SyntaxScopeId q) : category(c), target(t), qualifier(q), qualifier_category(c) {}
};
struct SyntaxScope {
    SyntaxScopeId parent = 0, nearest_namespace = 0, namespace_parent = 0;
    unsigned namespace_depth = 0;
    std::unordered_map<IdentifierId,SyntaxBinding> names;
    std::vector<SyntaxScopeId> imports;
    std::unordered_set<SyntaxScopeId> imported;
    std::uint64_t visited = 0;
};
// Syntax kinds, not serialized strings, are the interface to later semantics.
#define CPPGM_SYNTAX_KINDS(X) \
 X(Namespace,"namespace-definition") X(NamespaceAlias,"namespace-alias-definition") \
 X(UsingDirective,"using-directive") X(UsingDeclaration,"using-declaration") X(Target,"target") X(Inline,"inline") \
 X(Enum,"enum-specifier") X(EnumKey,"enum-key") X(Enumerator,"enumerator") \
 X(TranslationUnit,"translation-unit") X(EmptyDeclaration,"empty-declaration") \
 X(SimpleDeclaration,"simple-declaration") X(FunctionDefinition,"function-definition") \
 X(DeclSpecifiers,"decl-specifier-seq") X(DeclSpecifier,"decl-specifier") \
 X(Declarator,"declarator") X(AbstractDeclarator,"abstract-declarator") \
 X(Identifier,"identifier") X(Pointer,"ptr-operator") X(CvQualifier,"cv-qualifier") \
 X(NestedDeclarator,"nested-declarator") X(ArraySuffix,"array-suffix") \
 X(Parameters,"parameter-clause") X(Parameter,"parameter-declaration") \
 X(ParameterPack,"parameter-pack") X(DefaultArgument,"default-argument") \
 X(FunctionQualifier,"function-qualifier") X(TrailingReturn,"trailing-return-type") \
 X(InitDeclarators,"init-declarator-list") X(InitDeclarator,"init-declarator") \
 X(Initializer,"initializer") X(ParenInitializer,"paren-initializer") \
 X(BracedInit,"braced-init-list") X(TypeId,"type-id") X(TypeSpecifiers,"type-specifier-seq") \
 X(Decltype,"decltype-specifier") X(TypeSpecifier,"type-specifier") X(TypeName,"type-name") \
 X(ClassForward,"class-forward-declaration") X(ClassKey,"class-key") X(Linkage,"linkage-specification") X(Alias,"alias-declaration") X(StaticAssert,"static-assert-declaration") X(Message,"message") \
 X(Compound,"compound-statement") X(ExpressionStatement,"expression-statement") \
 X(Return,"return-statement") X(Break,"break-statement") X(Continue,"continue-statement") \
 X(Goto,"goto-statement") X(Label,"labeled-statement") X(Case,"case-statement") \
 X(Default,"default-statement") X(If,"if-statement") X(Then,"then") X(Else,"else") \
 X(Condition,"condition") X(ConditionDeclaration,"condition-declaration") \
 X(While,"while-statement") X(Do,"do-statement") X(Switch,"switch-statement") \
 X(For,"for-statement") X(ForInit,"for-init-statement") X(Iteration,"iteration") \
 X(RangeFor,"range-for-statement") X(RangeDeclaration,"range-declaration") \
 X(RangeInitializer,"range-initializer") \
 X(IdExpression,"id-expression") X(Literal,"literal") X(KeywordLiteral,"keyword-literal") \
 X(This,"this-expression") X(Parenthesized,"parenthesized-expression") \
 X(Unary,"unary-expression") X(Binary,"binary-expression") X(Assignment,"assignment-expression") \
 X(Conditional,"conditional-expression") X(Comma,"comma-expression") \
 X(Call,"call-expression") X(Arguments,"argument-list") X(ParenArguments,"paren-argument-list") X(Subscript,"subscript-expression") \
 X(Member,"member-expression") X(Postfix,"postfix-expression") X(Cast,"cast-expression") \
 X(Sizeof,"sizeof-expression") X(SizeofPack,"sizeof-pack-expression") \
 X(Trait,"type-trait-expression") X(Throw,"throw-expression") \
 X(Conversion,"conversion-expression") X(Type,"type") \
 X(ExceptionSpecification,"exception-specification") X(ExceptionTypes,"type-id-list") \
 X(New,"new-expression") X(Delete,"delete-expression") X(GlobalScope,"global-scope") X(ArrayDelete,"array-delete") X(Placement,"placement") X(Capture,"capture") \
 X(Lambda,"lambda-expression") X(LambdaIntroducer,"lambda-introducer") X(LambdaDeclarator,"lambda-declarator") X(Mutable,"lambda-specifier") X(Noexcept,"noexcept-specification") \
 X(FunctionTry,"function-try-block") X(Try,"try-block") X(Handler,"handler") X(ExceptionDeclaration,"exception-declaration") X(Ellipsis,"ellipsis") X(ThrowStatement,"throw-statement")

enum class SyntaxKind : std::uint16_t {
#define X(name, text) name,
 CPPGM_SYNTAX_KINDS(X)
#undef X
};
enum class SyntaxPayload : unsigned char { none, identifier, token, spelling, literal, raw_token };
struct SyntaxNode {
    SyntaxKind kind;
    SyntaxPayload payload = SyntaxPayload::none;
    SimpleKind token = SimpleKind::KW_AUTO;
    IdentifierId name = 0;
    std::uint32_t first = 0, last = 0, offset = 0, length = 0, literal = 0;
    bool is_decltype = false, is_operator = false, operator_array = false, member_pointer = false, operator_literal = false, operator_conversion = false, global_scope = false, has_parentheses = false;
    SyntaxScopeId scope = 0, resolved_scope = 0;
    SyntaxCategory category = SyntaxCategory::unknown;
    IdentifierId terminal_name = 0;
    SourceRange range = {0,0};
    SourceLocation location = {0,0,0};
};
struct SyntaxLiteral {
    PostKind kind;
    FundamentalType type;
    IdentifierId suffix;
    std::array<unsigned char,16> scalar;
    std::uint32_t offset, length;
    std::size_t width, elements;
};
struct SyntaxEdge { NodeId child; std::uint32_t next; };
// TU-owned flat arenas. Destruction is iterative; no child owns an allocation.
class SyntaxTree {
public:
    std::vector<SyntaxScope> scopes; // TU-owned environments survive parser destruction.
    std::vector<SyntaxNode> nodes;
    std::vector<SyntaxEdge> edges;
    std::vector<SyntaxLiteral> literals;
    std::vector<char> spellings;
    std::vector<unsigned char> values;
    SourceLocation anchor = {0,1,1};
    SyntaxTree();
    NodeId node(SyntaxKind, SourceLocation = {0,0,0});
    NodeId name(SyntaxKind, IdentifierId, SourceLocation = {0,0,0});
    NodeId token(SyntaxKind, SimpleKind, SourceLocation = {0,0,0});
    NodeId text(SyntaxKind, const std::string&, SourceLocation = {0,0,0});
    NodeId literal(const PostToken&);
    void append(NodeId parent, NodeId child);
    NodeId child(NodeId parent) const;
    std::string compact(NodeId, const IdentifierTable&) const;
    void dump(std::ostream&, const IdentifierTable&, NodeId root) const;
};
int emit_ast(const std::string& output, const std::vector<std::string>& inputs, bool telemetry = false);
}
