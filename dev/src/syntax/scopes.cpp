#include "syntax/parser.h"
#include <algorithm>
#include <limits>
namespace cppgm {
SyntaxScopeId SyntaxParser::create_scope(SyntaxScopeId parent, bool namespace_scope) {
    SyntaxScopeId id=tree_.scopes.size(); tree_.scopes.emplace_back();
    auto& scope=tree_.scopes[id]; scope.parent=parent;
    scope.nearest_namespace=namespace_scope ? id : tree_.scopes[parent].nearest_namespace;
    scope.namespace_parent=tree_.scopes[parent].nearest_namespace;
    scope.namespace_depth=tree_.scopes[parent].namespace_depth+(namespace_scope ? 1 : 0);
    return id;
}
SyntaxScopeId SyntaxParser::common_namespace(SyntaxScopeId a, SyntaxScopeId b) const {
    a=tree_.scopes[a].nearest_namespace; b=tree_.scopes[b].nearest_namespace;
    while (!expected_ && (a!=b)) {
        if (tree_.scopes[a].namespace_depth>=tree_.scopes[b].namespace_depth) a=tree_.scopes[a].namespace_parent;
        else b=tree_.scopes[b].namespace_parent;
    }
    return a;
}
void SyntaxParser::import_scope(SyntaxScopeId owner, SyntaxScopeId target) {
    auto& env=tree_.scopes[owner];
    if (target && env.imported.insert(target).second) env.imports.push_back(target);
}
void SyntaxParser::enter() { enter(create_scope(active_.empty() ? 0 : active_.back(),active_.empty())); }
void SyntaxParser::enter(SyntaxScopeId scope) { active_.push_back(scope); }
void SyntaxParser::qualify_scope(SyntaxScopeId target) {
    // Out-of-class member templates retain their own parameter frames ahead
    // of class lookup. Share each immutable published name index; never copy
    // bindings or reconnect a retained template/class environment in place.
    std::vector<SyntaxScopeId> frames;
    for (auto s=tree_.scopes[active_.back()].parent;
         s && tree_.scopes[s].template_environment; s=tree_.scopes[s].parent)
        frames.push_back(s);
    for (auto i=frames.rbegin();i!=frames.rend();++i) {
        auto view=create_scope(target);
        tree_.scopes[view].names_owner=*i;
        target=view;
    }
    tree_.scopes[active_.back()].parent=target;
}
void SyntaxParser::leave() { active_.pop_back(); }
void SyntaxParser::bind(IdentifierId id, Category value, SyntaxScopeId target) {
    if (!id) return;
    auto& b=tree_.scopes[active_.back()].names[id];
    b.category=value; b.target=target; b.parameter_order=0;
    if (value==Category::type || value==Category::templ || value==Category::space) { b.qualifier=target; b.qualifier_category=value; }
}
SyntaxBinding SyntaxParser::lookup(IdentifierId id, SyntaxScopeId scope, bool parents, bool qualifier, bool namespace_only) {
    ++queries_;
    const auto serial=++lookup_serial_;
    auto parameter_limit=std::numeric_limits<std::uint32_t>::max();
    auto usable=[&](const SyntaxBinding& b) {
        if (b.parameter_order>parameter_limit) return false;
        if (namespace_only) return b.qualifier_category==Category::space;
        return !qualifier || b.qualifier_category!=Category::unknown;
    };
    auto selected=[&](SyntaxBinding b) {
        if (qualifier || namespace_only) { b.target=b.qualifier; b.category=b.qualifier_category; }
        return b;
    };
    // Only matches from nominated scopes are deferred. No per-query allocation
    // or scan proportional to all TU scopes. Unqualified using-directives take
    // effect at their nearest common namespace (C++11 7.3.4/2,4); qualified
    // lookup instead searches the nominated namespace without lexical parents.
    SyntaxIndex<SyntaxBinding> pending;
    for (;scope;scope=parents ? tree_.scopes[scope].parent : 0) {
        if (tree_.scopes[scope].parameter_prefix)
            parameter_limit=std::min(parameter_limit,tree_.scopes[scope].parameter_limit);
        auto names=tree_.scopes[scope].names_owner ? tree_.scopes[scope].names_owner : scope;
        auto direct=tree_.scopes[names].names.find(id);
        if (direct && usable(direct->second)) return selected(direct->second);
        // Base lookup is a class-local step, unlike using-directive nominations.
        // Generation stamps bound diamond/cyclic syntax visits to once/query.
        std::vector<SyntaxScopeId> base_work(tree_.scopes[scope].bases);
        while (!expected_ && (!base_work.empty())) {
            auto next=base_work.back(); base_work.pop_back(); auto& env=tree_.scopes[next];
            if (env.visited==serial) continue;
            env.visited=serial;
            auto found=env.names.find(id);
            if (found && usable(found->second)) return selected(found->second);
            for (auto base:env.bases) base_work.push_back(base);
        }
        std::vector<SyntaxScopeId> work;
        for (auto imported:tree_.scopes[scope].imports) work.push_back(imported);
        while (!expected_ && (!work.empty())) {
            auto next=work.back(); work.pop_back(); auto& env=tree_.scopes[next];
            if (env.visited==serial) continue;
            env.visited=serial;
            auto found=env.names.find(id);
            if (found && usable(found->second)) {
                if (!parents) return selected(found->second);
                pending.insert(common_namespace(scope,next),found->second);
            }
            for (auto imported:env.imports) work.push_back(imported);
        }
        auto found=pending.find(scope);
        if (found) return selected(found->second);
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
        !(peek(offset).kind==PostKind::identifier && (at(SimpleKind::OP_COLON2,offset+1) ||
          (at(SimpleKind::OP_LT,offset+1) && (category(peek(offset).identifier)==Category::templ ||
           (lookup(peek(offset).identifier,active_.back()).category==Category::unknown && type_start(offset+2))))))) return;
    std::deque<InputToken> prefix;
    while (!expected_ && (offset--)) { prefix.push_back(std::move(lookahead_.front())); lookahead_.pop_front(); }
    auto original=lookahead_.front();
    NodeId n=qualified_raw(SyntaxKind::Identifier);
    original.kind=PostKind::identifier; original.identifier=tree_.nodes[n].terminal_name; original.name_node=n;
    lookahead_.push_front(std::move(original));
    while (!expected_ && (!prefix.empty())) { lookahead_.push_front(std::move(prefix.back())); prefix.pop_back(); }
}
NodeId SyntaxParser::namespace_declaration() {
    if (expected_) return 0;
    bool inlined=eat(SimpleKind::KW_INLINE); if (!require(SimpleKind::KW_NAMESPACE)) { return 0; }
    NodeId result=0; IdentifierId id=0;
    if (peek().kind==PostKind::identifier) { result=name(SyntaxKind::Namespace); id=tree_.nodes[result].name; }
    else result=tree_.text(SyntaxKind::Namespace,"<unnamed>");
    if (eat(SimpleKind::OP_ASS)) {
        if (!id || inlined) return error("namespace alias name");
        tree_.nodes[result].kind=SyntaxKind::NamespaceAlias;
        NodeId target=qualified(SyntaxKind::Target,true); tree_.append(result,target); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
        bind(id,Category::space,tree_.nodes[target].resolved_scope); return result;
    }
    auto& names=tree_.scopes[active_.back()].names;
    auto old=names.find(id);
    SyntaxScopeId scope=old && old->second.category==Category::space ? old->second.target : 0;
    if (!scope) scope=create_scope(active_.back(),true);
    bind(id,Category::space,scope); tree_.nodes[result].scope=scope;
    // An unnamed namespace is reopened through a reserved zero identifier.
    if (!id) tree_.scopes[active_.back()].names[0]={Category::space,scope,scope};
    if (inlined || !id) import_scope(active_.back(),scope);
    if (inlined) tree_.append(result,tree_.node(SyntaxKind::Inline));
    if (!require(SimpleKind::OP_LBRACE)) { return 0; } enter(scope);
    while (!expected_ && (!at(SimpleKind::OP_RBRACE))) {
        if (peek().kind==PostKind::eof) return error("namespace closing brace");
        tree_.append(result,declaration());
    }
    take(); leave(); return result;
}
NodeId SyntaxParser::using_declaration() {
    if (expected_) return 0;
    if (!require(SimpleKind::KW_USING)) { return 0; }
    if (eat(SimpleKind::KW_NAMESPACE)) {
        NodeId result=tree_.node(SyntaxKind::UsingDirective), target=qualified(SyntaxKind::Target,true);
        tree_.append(result,target); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
        auto scope=tree_.nodes[target].resolved_scope;
        import_scope(active_.back(),scope);
        return result;
    }
    if (peek().kind==PostKind::identifier && at(SimpleKind::OP_ASS,1)) {
        auto id=take(); take(); NodeId result=leaf(SyntaxKind::Alias,id), type=type_id(false,true);
        tree_.append(result,type); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
        NodeId spec=tree_.child(tree_.child(type));
        // Class/enum specifiers own a scope; qualified type names resolve one.
        // Preserve that identity across alias syntax without rendering its name.
        const auto& base=tree_.nodes[spec];
        bind(id.identifier,Category::type,base.resolved_scope ? base.resolved_scope : base.scope); return result;
    }
    bool typename_name=eat(SimpleKind::KW_TYPENAME);
    NodeId result=tree_.node(SyntaxKind::UsingDeclaration), target=qualified(SyntaxKind::Target);
    tree_.append(result,target); if (!require(SimpleKind::OP_SEMICOLON)) { return 0; }
    const auto& n=tree_.nodes[target];
    bind(n.terminal_name,typename_name ? Category::type : n.category,n.resolved_scope);
    return result;
}
NodeId SyntaxParser::enum_specifier() {
    if (expected_) return 0;
    auto keyword=take(); NodeId key=0;
    if (at(SimpleKind::KW_CLASS) || at(SimpleKind::KW_STRUCT)) key=leaf(SyntaxKind::EnumKey,take());
    NodeId result=peek().kind==PostKind::identifier ? qualified(SyntaxKind::Enum) : tree_.node(SyntaxKind::Enum,keyword.location);
    IdentifierId id=tree_.nodes[result].terminal_name;
    if (!id && !at(SimpleKind::OP_LBRACE)) return error("enum name or body");
    if (key) tree_.append(result,key);
    auto prior=id ? lookup(id,active_.back(),false) : SyntaxBinding{};
    auto scope=prior.target ? prior.target : create_scope(active_.back());
    tree_.nodes[result].scope=scope;
    bind(id,Category::type,scope);
    if (eat(SimpleKind::OP_COLON)) tree_.append(result,type_id());
    if (!eat(SimpleKind::OP_LBRACE)) return result;
    enter(scope);
    while (!expected_ && (!at(SimpleKind::OP_RBRACE))) {
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
    if (!require(SimpleKind::OP_RBRACE)) { return 0; } leave();
    return result;
}
}
