#include "syntax/parser.h"
namespace cppgm {
SyntaxScopeId SyntaxParser::create_scope(SyntaxScopeId parent) {
    SyntaxScopeId id=tree_.scopes.size(); tree_.scopes.emplace_back(); tree_.scopes[id].parent=parent; return id;
}
void SyntaxParser::import_scope(SyntaxScopeId owner, SyntaxScopeId target) {
    auto& env=tree_.scopes[owner];
    if (target && env.imported.insert(target).second) env.imports.push_back(target);
}
void SyntaxParser::enter() { enter(create_scope(active_.empty() ? 0 : active_.back())); }
void SyntaxParser::enter(SyntaxScopeId scope) { active_.push_back(scope); }
void SyntaxParser::leave() { active_.pop_back(); }
void SyntaxParser::bind(IdentifierId id, Category value, SyntaxScopeId target) {
    if (!id) return;
    auto& b=tree_.scopes[active_.back()].names[id];
    b.category=value; b.target=target;
    if (value==Category::type || value==Category::templ || value==Category::space) b.qualifier=target;
}
SyntaxBinding SyntaxParser::lookup(IdentifierId id, SyntaxScopeId scope, bool parents, bool qualifier) {
    ++queries_;
    const auto serial=++lookup_serial_;
    for (;scope;scope=parents ? tree_.scopes[scope].parent : 0) {
        auto direct=tree_.scopes[scope].names.find(id);
        if (direct!=tree_.scopes[scope].names.end() && (!qualifier || direct->second.qualifier)) {
            auto result=direct->second;
            if (qualifier) result.target=result.qualifier;
            return result;
        }
        // Visit only nominated scopes and their edges. The generation stamp is
        // per environment, not a per-query TU-sized bitmap or cache.
        std::vector<SyntaxScopeId> work;
        for (auto imported:tree_.scopes[scope].imports) work.push_back(imported);
        while (!work.empty()) {
            auto next=work.back(); work.pop_back(); auto& env=tree_.scopes[next];
            if (env.visited==serial) continue;
            env.visited=serial;
            auto found=env.names.find(id);
            if (found!=env.names.end() && (!qualifier || found->second.qualifier)) {
                auto result=found->second;
                if (qualifier) result.target=result.qualifier;
                return result;
            }
            for (auto imported:env.imports) work.push_back(imported);
        }
    }
    return {};
}
SyntaxParser::Category SyntaxParser::hint(IdentifierId id) {
    if (hints_.size()<=id) hints_.resize(id+1,Category::unknown);
    if (hints_[id]!=Category::unknown) return hints_[id];
    auto spelling=ids_.spelling(id); Category result=Category::value;
    for (std::size_t i=0;i<spelling.size;++i) {
        if (spelling.data[i]=='T') { result=Category::templ; break; }
        if (spelling.data[i]=='C' || spelling.data[i]=='Y' || spelling.data[i]=='E') result=Category::type;
    }
    return hints_[id]=result;
}
SyntaxParser::Category SyntaxParser::category(IdentifierId id) {
    auto b=lookup(id,active_.back()); return b.category==Category::unknown ? hint(id) : b.category;
}
void SyntaxParser::prepare_name(unsigned offset) {
    // Factor a qualified prefix once. Replace just that grammatical prefix with
    // its owned graph handle; no replay, whole-TU token vector or string lookup.
    peek(offset);
    if (lookahead_[offset].name_node) return;
    if (at(SimpleKind::OP_COLON2,offset) && peek(offset+1).kind!=PostKind::identifier) return;
    if (!at(SimpleKind::OP_COLON2,offset) &&
        !(peek(offset).kind==PostKind::identifier && at(SimpleKind::OP_COLON2,offset+1))) return;
    std::deque<InputToken> prefix;
    while (offset--) { prefix.push_back(std::move(lookahead_.front())); lookahead_.pop_front(); }
    auto original=lookahead_.front();
    NodeId n=qualified_raw(SyntaxKind::Identifier);
    original.kind=PostKind::identifier; original.identifier=tree_.nodes[n].terminal_name; original.name_node=n;
    lookahead_.push_front(std::move(original));
    while (!prefix.empty()) { lookahead_.push_front(std::move(prefix.back())); prefix.pop_back(); }
}
NodeId SyntaxParser::namespace_declaration() {
    bool inlined=eat(SimpleKind::KW_INLINE); require(SimpleKind::KW_NAMESPACE);
    NodeId result=0; IdentifierId id=0;
    if (peek().kind==PostKind::identifier) { result=name(SyntaxKind::Namespace); id=tree_.nodes[result].name; }
    else result=tree_.text(SyntaxKind::Namespace,"<unnamed>");
    if (eat(SimpleKind::OP_ASS)) {
        if (!id || inlined) error("namespace alias name");
        tree_.nodes[result].kind=SyntaxKind::NamespaceAlias;
        NodeId target=qualified(SyntaxKind::Target); tree_.append(result,target); require(SimpleKind::OP_SEMICOLON);
        bind(id,Category::space,tree_.nodes[target].resolved_scope); return result;
    }
    auto& names=tree_.scopes[active_.back()].names;
    auto old=names.find(id);
    SyntaxScopeId scope=old!=names.end() && old->second.category==Category::space ? old->second.target : 0;
    if (!scope) scope=create_scope(active_.back());
    bind(id,Category::space,scope); tree_.nodes[result].scope=scope;
    // An unnamed namespace is reopened through a reserved zero identifier.
    if (!id) tree_.scopes[active_.back()].names[0]={Category::space,scope,scope};
    if (inlined || !id) import_scope(active_.back(),scope);
    if (inlined) tree_.append(result,tree_.node(SyntaxKind::Inline));
    require(SimpleKind::OP_LBRACE); enter(scope);
    while (!at(SimpleKind::OP_RBRACE)) {
        if (peek().kind==PostKind::eof) error("namespace closing brace");
        tree_.append(result,declaration());
    }
    take(); leave(); return result;
}
NodeId SyntaxParser::using_declaration() {
    require(SimpleKind::KW_USING);
    if (eat(SimpleKind::KW_NAMESPACE)) {
        NodeId result=tree_.node(SyntaxKind::UsingDirective), target=qualified(SyntaxKind::Target);
        tree_.append(result,target); require(SimpleKind::OP_SEMICOLON);
        auto scope=tree_.nodes[target].resolved_scope;
        import_scope(active_.back(),scope);
        return result;
    }
    if (peek().kind==PostKind::identifier && at(SimpleKind::OP_ASS,1)) {
        auto id=take(); take(); NodeId result=leaf(SyntaxKind::Alias,id), type=type_id(false,true);
        tree_.append(result,type); require(SimpleKind::OP_SEMICOLON);
        NodeId spec=tree_.child(tree_.child(type));
        bind(id.identifier,Category::type,tree_.nodes[spec].resolved_scope); return result;
    }
    bool typename_name=eat(SimpleKind::KW_TYPENAME);
    NodeId result=tree_.node(SyntaxKind::UsingDeclaration), target=qualified(SyntaxKind::Target);
    tree_.append(result,target); require(SimpleKind::OP_SEMICOLON);
    const auto& n=tree_.nodes[target];
    bind(n.terminal_name,typename_name ? Category::type : n.category,n.resolved_scope);
    return result;
}
NodeId SyntaxParser::enum_specifier() {
    auto keyword=take(); NodeId key=0;
    if (at(SimpleKind::KW_CLASS) || at(SimpleKind::KW_STRUCT)) key=leaf(SyntaxKind::EnumKey,take());
    NodeId result=peek().kind==PostKind::identifier ? qualified(SyntaxKind::Enum) : tree_.node(SyntaxKind::Enum,keyword.location);
    IdentifierId id=tree_.nodes[result].terminal_name;
    if (!id && !at(SimpleKind::OP_LBRACE)) error("enum name or body");
    if (key) tree_.append(result,key);
    auto prior=id ? lookup(id,active_.back(),false) : SyntaxBinding{};
    auto scope=prior.target ? prior.target : create_scope(active_.back());
    tree_.nodes[result].scope=scope;
    bind(id,Category::type,scope);
    if (eat(SimpleKind::OP_COLON)) tree_.append(result,type_id());
    if (!eat(SimpleKind::OP_LBRACE)) return result;
    enter(scope);
    while (!at(SimpleKind::OP_RBRACE)) {
        NodeId n=name(SyntaxKind::Enumerator); auto enumerator=tree_.nodes[n].name;
        bind(enumerator,Category::value);
        if (!key) {
            // Unscoped enumerators are declarations in the containing scope,
            // not using edges that make every future lookup scan all enums.
            auto parent=tree_.scopes[scope].parent;
            auto& b=tree_.scopes[parent].names[enumerator];
            b.category=Category::value; b.target=0;
        }
        if (eat(SimpleKind::OP_ASS)) tree_.append(n,expression(2));
        tree_.append(result,n);
        if (!eat(SimpleKind::OP_COMMA)) break;
    }
    require(SimpleKind::OP_RBRACE); leave();
    return result;
}
}
