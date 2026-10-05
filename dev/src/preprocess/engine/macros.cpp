#include "preprocess/engine/preprocessor.h"
#include <algorithm>
#include <stdexcept>

namespace cppgm {
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
LexerOptions options() { LexerOptions o; o.collect_literal_elements = true; o.convert_empty_character = true; o.generated_token = true; return o; }
bool hash(const PPItem& p) { return punctuation(p,"#") || punctuation(p,"%:"); }
bool paste(const PPItem& p) { return punctuation(p,"##") || punctuation(p,"%:%:"); }
}
bool punctuation(const PPItem& p, const char* s) { return p.token.kind == TokenKind::punctuator && p.text == s; }
PPItem capture(Lexer& l, Token t) {
    PPItem p; p.token = t; p.text = l.spelling();
    if (t.kind == TokenKind::string || t.kind == TokenKind::ud_string ||
        t.kind == TokenKind::character || t.kind == TokenKind::ud_character) {
        p.elements = l.literal_elements(); p.literal_end = l.literal_end();
        p.physical_end = l.literal_physical_end(); p.suffix_location = l.literal_suffix_location();
    }
    p.line = t.location.line;
    return p;
}
MacroEngine::MacroEngine(IdentifierTable& ids, PreprocessorMetrics& m) : identifiers_(ids), metrics_(m) {
    va_ = ids.intern("__VA_ARGS__"); paints_.push_back({{0,0}}); paints_.push_back({{0,0}});
}
bool MacroEngine::defined(IdentifierId id) const { return id < bindings_.size() && bindings_[id]; }
bool MacroEngine::painted(std::uint32_t p, IdentifierId id) const {
    // Persistent binary radix sets: membership/insertion has a fixed 32-bit
    // bound even on long alias chains. Union/intersection share equal subtrees.
    for (unsigned bit=0;p && bit<32;++bit) p=paints_[p].child[(id>>bit)&1];
    return p!=0;
}
std::uint32_t MacroEngine::insert_paint(std::uint32_t p, IdentifierId id, unsigned bit) {
    if (bit==32) return 1;
    Paint node=p?paints_[p]:Paint{{0,0}};
    unsigned side=(id>>bit)&1;
    node.child[side]=insert_paint(node.child[side],id,bit+1);
    paints_.push_back(node); return paints_.size()-1;
}
std::uint32_t MacroEngine::add_paint(std::uint32_t p, IdentifierId id) {
    return painted(p,id)?p:insert_paint(p,id,0);
}
std::uint32_t MacroEngine::merge_paint(std::uint32_t a, std::uint32_t b, unsigned bit) {
    if (!a || a==b) return b;
    if (!b || bit==32) return a;
    Paint node={{merge_paint(paints_[a].child[0],paints_[b].child[0],bit+1),
                 merge_paint(paints_[a].child[1],paints_[b].child[1],bit+1)}};
    if(node.child[0]==paints_[a].child[0] && node.child[1]==paints_[a].child[1]) return a;
    if(node.child[0]==paints_[b].child[0] && node.child[1]==paints_[b].child[1]) return b;
    paints_.push_back(node); return paints_.size()-1;
}
std::uint32_t MacroEngine::intersect_paint(std::uint32_t a, std::uint32_t b, unsigned bit) {
    if (!a || !b) return 0;
    if (a==b || bit==32) return a;
    Paint node={{intersect_paint(paints_[a].child[0],paints_[b].child[0],bit+1),
                 intersect_paint(paints_[a].child[1],paints_[b].child[1],bit+1)}};
    if(!node.child[0] && !node.child[1]) return 0;
    if(node.child[0]==paints_[a].child[0] && node.child[1]==paints_[a].child[1]) return a;
    if(node.child[0]==paints_[b].child[0] && node.child[1]==paints_[b].child[1]) return b;
    paints_.push_back(node); return paints_.size()-1;
}
PPItem MacroEngine::synthetic(const std::string& text, const PPItem& origin) {
    SourceBuffer source(text); Lexer lexer(source,identifiers_, options());
    Token t = lexer.next();
    require(t.kind != TokenKind::eof && t.kind != TokenKind::newline && t.kind != TokenKind::whitespace, "empty generated token");
    PPItem p = capture(lexer,t);
    Token end = lexer.next(); if (end.kind == TokenKind::newline) end = lexer.next();
    require(end.kind == TokenKind::eof, "paste does not form one preprocessing token");
    p.token.location = origin.token.location; p.token.range = origin.token.range;
    p.file = origin.file; p.line = origin.line; p.space = origin.space; p.paint = origin.paint;
    return p;
}
void MacroEngine::define(const std::vector<PPItem>& v) {
    require(!v.empty() && v[0].token.kind == TokenKind::identifier && v[0].token.identifier != va_, "invalid macro name");
    Macro m; m.name = v[0].token.identifier;
    std::size_t i = 1;
    if (i < v.size() && punctuation(v[i],"(") && !v[i].space) {
        m.function = true; ++i;
        if (i < v.size() && !punctuation(v[i],")")) for (;;) {
            require(i < v.size(), "unterminated macro parameters");
            if (punctuation(v[i],"...")) { m.variadic = true; m.parameters.push_back(va_); ++i; break; }
            require(v[i].token.kind == TokenKind::identifier && v[i].token.identifier != va_, "invalid macro parameter");
            IdentifierId id = v[i++].token.identifier;
            require(std::find(m.parameters.begin(),m.parameters.end(),id) == m.parameters.end(), "duplicate macro parameter");
            m.parameters.push_back(id);
            require(i < v.size(), "unterminated macro parameters");
            if (punctuation(v[i],")")) break;
            require(punctuation(v[i++],","), "expected parameter comma");
        }
        require(i < v.size() && punctuation(v[i],")"), "expected macro parameter close"); ++i;
    } else if (i < v.size()) require(v[i].space, "object macro requires whitespace");
    for (; i < v.size(); ++i) {
        Replacement r; r.item = v[i];
        require(v[i].token.identifier != va_ || m.variadic, "__VA_ARGS__ outside variadic macro");
        if (v[i].token.kind == TokenKind::identifier) {
            auto p = std::find(m.parameters.begin(), m.parameters.end(), v[i].token.identifier);
            if (p != m.parameters.end()) r.parameter = p-m.parameters.begin();
        }
        m.replacement.push_back(std::move(r));
    }
    for (std::size_t j=0;j<m.replacement.size();++j) {
        const auto& r = m.replacement[j];
        if (paste(r.item)) require(j && j+1<m.replacement.size(), "## at replacement edge");
        if (m.function && hash(r.item)) require(j+1<m.replacement.size() && m.replacement[j+1].parameter>=0, "# must precede parameter");
    }
    if (defined(m.name)) {
        const auto& old = definitions_[bindings_[m.name]-1];
        bool same = old.function == m.function && old.variadic == m.variadic && old.parameters == m.parameters && old.replacement.size()==m.replacement.size();
        for (std::size_t j=0;same && j<m.replacement.size();++j)
            same = old.replacement[j].item.text == m.replacement[j].item.text && (j==0 || old.replacement[j].item.space == m.replacement[j].item.space);
        require(same,"incompatible macro redefinition"); return;
    }
    if (bindings_.size()<=m.name) bindings_.resize(m.name+1);
    bindings_[m.name] = definitions_.size()+1; definitions_.push_back(std::move(m));
}
void MacroEngine::undefine(const std::vector<PPItem>& v) {
    require(v.size()==1 && v[0].token.kind==TokenKind::identifier && v[0].token.identifier!=va_, "invalid undef");
    if (v[0].token.identifier < bindings_.size()) bindings_[v[0].token.identifier]=0;
}
void MacroEngine::substitute(const Macro& m, const PPItem& head, std::uint32_t replacement_paint, const std::vector<std::vector<PPItem>>& args, std::vector<PPItem>& out, const Builtin& builtin) {
    std::vector<std::vector<PPItem>> expanded(args.size());
    std::vector<bool> ready(args.size(),false);
    const std::uint32_t paint_id = add_paint(replacement_paint,m.name);
    const std::uint32_t parameter_paint = add_paint(head.paint,m.name);
    bool join = false;
    for (std::size_t i=0;i<m.replacement.size();++i) {
        const auto& r = m.replacement[i];
        if (paste(r.item)) { join = true; continue; }
        std::vector<PPItem> part;
        if (m.function && hash(r.item)) {
            const auto& raw = args[m.replacement[++i].parameter];
            std::string s = "\"";
            for (std::size_t j=0;j<raw.size();++j) {
                if (j && raw[j].space) s += ' ';
                const bool literal = raw[j].token.kind==TokenKind::string || raw[j].token.kind==TokenKind::ud_string || raw[j].token.kind==TokenKind::character || raw[j].token.kind==TokenKind::ud_character;
                for (char c : raw[j].text) { if (literal && (c=='\\' || c=='\"')) s += '\\'; s += c; }
            }
            s += '"'; part.push_back(synthetic(s,head));
        } else if (r.parameter>=0) {
            const std::size_t p = r.parameter;
            const bool comma_extension = m.variadic && p+1==m.parameters.size() && join && !out.empty() && punctuation(out.back(),",");
            if (comma_extension) {
                join=false;
                if(args[p].empty()) {out.pop_back(); continue;}
            }
            const bool raw = join || (i+1<m.replacement.size() && paste(m.replacement[i+1].item));
            if (!raw && !ready[p]) { expanded[p] = expand(args[p], builtin); ready[p]=true; }
            part = raw ? args[p] : expanded[p];
        } else part.push_back(r.item);
        // Empty arguments adjacent to ## are explicit placemarkers.
        if (part.empty() && (join || (i+1<m.replacement.size() && paste(m.replacement[i+1].item)))) {
            PPItem empty; empty.token.kind=TokenKind::other; part.push_back(std::move(empty));
        }
        for (auto& p : part) {
            if(r.parameter<0) p.paint=merge_paint(p.paint,paint_id);
            else {
                // Argument prescan's nesting does not survive substitution.
                // Permanently suppressed tokens retain blocked independently.
                // An unexpanded parameter instead inherits invocation nesting
                // (g(f)(g)(3)); its source location remains argument-owned.
                p.paint=p.paint?0:parameter_paint;
            }
            if (r.parameter<0) { p.file=head.file; p.line=head.line;
                p.token.location=head.token.location;
            }
        }
        if (!part.empty()) part.front().space = r.item.space;
        if (join) {
            require(!out.empty() && !part.empty(), "missing paste operand");
            PPItem lhs = std::move(out.back()); out.pop_back();
            if (lhs.text.empty()) part.front().space = lhs.space;
            else if (part.front().text.empty()) part.front() = std::move(lhs);
            else {
                ++metrics_.paste_tokens;
                PPItem pasted = synthetic(lhs.text+part.front().text,head);
                pasted.paint=merge_paint(lhs.paint,part.front().paint); pasted.space=lhs.space;
                part.front()=std::move(pasted);
            }
            join = false;
        }
        out.insert(out.end(),std::make_move_iterator(part.begin()),std::make_move_iterator(part.end()));
    }
    out.erase(std::remove_if(out.begin(),out.end(),[](const PPItem& p){ return p.text.empty(); }),out.end());
    if (!out.empty()) out.front().space=head.space;
}
bool MacroEngine::next(PPItem& result, std::vector<PPItem>& pending, const Pull& source, const Builtin& builtin) {
    auto pull = [&](PPItem& p) {
        if (!pending.empty()) { p=std::move(pending.back()); pending.pop_back(); return true; }
        return source(p);
    };
    PPItem p;
    while (pull(p)) {
        ++metrics_.expanded;
        if (p.token.kind!=TokenKind::identifier || p.blocked) { result=std::move(p); return true; }
        require(p.token.identifier != va_, "__VA_ARGS__ outside replacement list");
        ++metrics_.lookups;
        if (builtin && builtin(p)) { result=std::move(p); return true; }
        const IdentifierId id = p.token.identifier;
        if (!defined(id)) { result=std::move(p); return true; }
        const Macro& m = definitions_[bindings_[id]-1];
        std::vector<std::vector<PPItem>> args;
        PPItem invocation=p;
        if (m.function) {
            PPItem open;
            if (!pull(open)) { result=std::move(p); return true; }
            if (!punctuation(open,"(")) { pending.push_back(std::move(open)); result=std::move(p); return true; }
            if (painted(p.paint,id)) { pending.push_back(std::move(open)); p.blocked=true; result=std::move(p); return true; }
            args.emplace_back(); std::size_t depth=0; PPItem t;
            for (;;) {
                require(pull(t), "unterminated macro invocation");
                if (!depth && punctuation(t,")")) { invocation.paint=intersect_paint(p.paint,t.paint); break; }
                if (!depth && punctuation(t,",")) { args.emplace_back(); continue; }
                if (punctuation(t,"(")) ++depth;
                if (punctuation(t,")")) --depth;
                ++metrics_.argument_tokens; args.back().push_back(std::move(t));
            }
            if (args.size()==1 && args[0].empty() && m.parameters.empty()) args.clear();
            if (m.variadic) {
                require(args.size()>=m.parameters.size(), "too few variadic arguments");
                std::size_t fixed=m.parameters.size()-1;
                for (std::size_t j=fixed+1;j<args.size();++j) {
                    PPItem comma=synthetic(",",p); comma.space=false; args[fixed].push_back(std::move(comma));
                    args[fixed].insert(args[fixed].end(),std::make_move_iterator(args[j].begin()),std::make_move_iterator(args[j].end()));
                }
                args.resize(m.parameters.size());
            } else require(args.size()==m.parameters.size(), "wrong macro argument count");
        }
        if (!m.function && painted(p.paint,id)) { p.blocked=true; result=std::move(p); return true; }
        ++metrics_.invocations;
        std::vector<PPItem> replacement;
        substitute(m,p,invocation.paint,args,replacement,builtin);
        for (auto i=replacement.rbegin();i!=replacement.rend();++i) pending.push_back(std::move(*i));
        metrics_.max_pending=std::max(metrics_.max_pending,pending.size());
    }
    return false;
}
std::vector<PPItem> MacroEngine::expand(const std::vector<PPItem>& input, const Builtin& builtin) {
    std::size_t pos=0; std::vector<PPItem> pending, out; PPItem p;
    Pull pull=[&](PPItem& t){ if(pos==input.size()) return false; t=input[pos++]; return true; };
    while (next(p,pending,pull,builtin)) out.push_back(std::move(p));
    return out;
}
}
