#include "preprocess/engine/preprocessor.h"
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace cppgm {
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
LexerOptions options() { LexerOptions o; o.collect_literal_elements = true; o.convert_empty_character = true; o.generated_token = true; return o; }
bool hash(const PPItem& p) { return punctuation(p,"#") || punctuation(p,"%:"); }
bool paste(const PPItem& p) { return punctuation(p,"##") || punctuation(p,"%:%:"); }
void inherit_origin(PPItem& item, const PPItem& origin) {
    item.file=origin.file; item.line=origin.line;
    item.token.location=origin.token.location; item.token.range=origin.token.range;
    item.token.generated=true;
    item.physical_end=origin.token.range.end; item.suffix_location=origin.token.location;
}
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
    paints_.push_back(node); metrics_.paint_nodes=paints_.size(); return paints_.size()-1;
}
void MacroEngine::grow_extensions() {
    extension_slots_.assign(extension_slots_.empty()?64:extension_slots_.size()*2,0);
    for(std::size_t i=0;i<extensions_.size();++i) {
        const auto& e=extensions_[i];
        std::uint64_t hash=(std::uint64_t(e.paint)<<32)|e.macro;
        hash^=hash>>33; hash*=0xff51afd7ed558ccdULL; hash^=hash>>33;
        std::size_t slot=hash&(extension_slots_.size()-1);
        while(extension_slots_[slot]) slot=(slot+1)&(extension_slots_.size()-1);
        extension_slots_[slot]=i+1;
    }
}
std::uint32_t MacroEngine::add_paint(std::uint32_t p, IdentifierId id) {
    ++metrics_.paint_queries;
    if(extension_slots_.empty() || (extensions_.size()+1)*4>extension_slots_.size()*3) grow_extensions();
    std::uint64_t hash=(std::uint64_t(p)<<32)|id;
    hash^=hash>>33; hash*=0xff51afd7ed558ccdULL; hash^=hash>>33;
    std::size_t slot=hash&(extension_slots_.size()-1);
    while(extension_slots_[slot]) {
        const auto& e=extensions_[extension_slots_[slot]-1];
        if(e.paint==p && e.macro==id) {++metrics_.paint_cache_hits; return e.result;}
        slot=(slot+1)&(extension_slots_.size()-1);
    }
    std::uint32_t result=painted(p,id)?p:insert_paint(p,id,0);
    extensions_.push_back({p,id,result}); extension_slots_[slot]=extensions_.size();
    return result;
}
std::uint32_t MacroEngine::merge_paint(std::uint32_t a, std::uint32_t b, unsigned bit) {
    if (!a || a==b) return b;
    if (!b || bit==32) return a;
    Paint node={{merge_paint(paints_[a].child[0],paints_[b].child[0],bit+1),
                 merge_paint(paints_[a].child[1],paints_[b].child[1],bit+1)}};
    if(node.child[0]==paints_[a].child[0] && node.child[1]==paints_[a].child[1]) return a;
    if(node.child[0]==paints_[b].child[0] && node.child[1]==paints_[b].child[1]) return b;
    paints_.push_back(node); metrics_.paint_nodes=paints_.size(); return paints_.size()-1;
}
std::uint32_t MacroEngine::intersect_paint(std::uint32_t a, std::uint32_t b, unsigned bit) {
    if (!a || !b) return 0;
    if (a==b || bit==32) return a;
    Paint node={{intersect_paint(paints_[a].child[0],paints_[b].child[0],bit+1),
                 intersect_paint(paints_[a].child[1],paints_[b].child[1],bit+1)}};
    if(!node.child[0] && !node.child[1]) return 0;
    if(node.child[0]==paints_[a].child[0] && node.child[1]==paints_[a].child[1]) return a;
    if(node.child[0]==paints_[b].child[0] && node.child[1]==paints_[b].child[1]) return b;
    paints_.push_back(node); metrics_.paint_nodes=paints_.size(); return paints_.size()-1;
}
PPItem MacroEngine::synthetic(const std::string& text, const PPItem& origin) {
    SourceBuffer source(text); Lexer lexer(source,identifiers_, options());
    Token t = lexer.next();
    require(t.kind != TokenKind::eof && t.kind != TokenKind::newline && t.kind != TokenKind::whitespace, "empty generated token");
    PPItem p = capture(lexer,t);
    Token end = lexer.next(); if (end.kind == TokenKind::newline) end = lexer.next();
    require(end.kind == TokenKind::eof, "paste does not form one preprocessing token");
    inherit_origin(p,origin); p.space = origin.space; p.paint = origin.paint;
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
            if (punctuation(v[i],"...")) { m.variadic = true;
                if(parameter_slots_.size()<=va_) parameter_slots_.resize(va_+1,-1);
                parameter_slots_[va_]=m.parameters.size(); m.parameters.push_back(va_); ++i; break; }
            require(v[i].token.kind == TokenKind::identifier && v[i].token.identifier != va_, "invalid macro parameter");
            IdentifierId id = v[i++].token.identifier;
            if(parameter_slots_.size()<=id) parameter_slots_.resize(id+1,-1);
            require(parameter_slots_[id]<0,"duplicate macro parameter");
            parameter_slots_[id]=m.parameters.size();
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
            const IdentifierId id=v[i].token.identifier;
            if(id<parameter_slots_.size()) r.parameter=parameter_slots_[id];
        }
        m.replacement.push_back(std::move(r));
    }
    for(IdentifierId id:m.parameters) parameter_slots_[id]=-1;
    for (std::size_t j=0;j<m.replacement.size();++j) {
        const auto& r = m.replacement[j];
        if (paste(r.item)) require(j && j+1<m.replacement.size(), "## at replacement edge");
        if (m.function && hash(r.item)) require(j+1<m.replacement.size() && m.replacement[j+1].parameter>=0, "# must precede parameter");
    }
    std::vector<bool> prescan(m.parameters.size(),false);
    for(std::size_t j=0;j<m.replacement.size();++j) {
        const auto& r=m.replacement[j];
        if(r.parameter<0 || (j && hash(m.replacement[j-1].item))) continue;
        const bool comma_extension=m.variadic && std::size_t(r.parameter)+1==m.parameters.size() && j>1 && paste(m.replacement[j-1].item) && punctuation(m.replacement[j-2].item,",");
        if(comma_extension || ((j==0 || !paste(m.replacement[j-1].item)) && (j+1==m.replacement.size() || !paste(m.replacement[j+1].item)))) prescan[r.parameter]=true;
    }
    for(std::size_t j=0;j<prescan.size();++j) if(prescan[j]) m.prescan_parameters.push_back(j);
    if (defined(m.name)) {
        const auto& old = definitions_[bindings_[m.name]-1];
        bool same = old.function == m.function && old.variadic == m.variadic && old.parameters == m.parameters && old.replacement.size()==m.replacement.size();
        for (std::size_t j=0;same && j<m.replacement.size();++j)
            same = old.replacement[j].item.text == m.replacement[j].item.text && (j==0 || old.replacement[j].item.space == m.replacement[j].item.space);
        require(same,"incompatible macro redefinition"); return;
    }
    if (bindings_.size()<=m.name) bindings_.resize(m.name+1);
    std::uint32_t slot;
    if(free_definitions_.empty()) {slot=definitions_.size(); definitions_.emplace_back();}
    else {slot=free_definitions_.back();free_definitions_.pop_back();}
    bindings_[m.name]=slot+1; definitions_[slot]=std::move(m);
}
void MacroEngine::undefine(const std::vector<PPItem>& v) {
    require(v.size()==1 && v[0].token.kind==TokenKind::identifier && v[0].token.identifier!=va_, "invalid undef");
    IdentifierId id=v[0].token.identifier;
    if(defined(id)) {
        std::uint32_t slot=bindings_[id]-1;
        definitions_[slot]=Macro(); free_definitions_.push_back(slot); bindings_[id]=0;
    }
}
// Delimiter links are built exactly once per retained/generated sequence.
// Nested invocations use O(argument count) views, never scan or copy their
// descendants. A sequence lives only while an argument/task/chunk references it.
struct MacroEngine::Sequence {
    struct Link { std::size_t close, comma; };
    std::vector<PPItem> items;
    std::vector<Link> links;
};
std::shared_ptr<const MacroEngine::Sequence> MacroEngine::sequence(std::vector<PPItem> items) {
    std::shared_ptr<Sequence> result=std::make_shared<Sequence>();
    result->items=std::move(items);
    const auto absent=std::numeric_limits<std::size_t>::max();
    result->links.resize(result->items.size(),{absent,absent});
    struct Open {std::size_t index, separator;};
    std::vector<Open> stack;
    for(std::size_t i=0;i<result->items.size();++i) {
        const auto& p=result->items[i];
        if(punctuation(p,"(")) stack.push_back({i,i});
        else if(punctuation(p,",") && !stack.empty()) {
            result->links[stack.back().separator].comma=i; stack.back().separator=i;
        } else if(punctuation(p,")") && !stack.empty()) {
            auto open=stack.back();stack.pop_back();
            result->links[open.index].close=i; result->links[open.separator].comma=i;
        }
    }
    metrics_.indexed_tokens+=result->items.size();
    return result;
}
std::vector<MacroEngine::Span> MacroEngine::arguments(const Macro& m, const std::shared_ptr<const Sequence>& seq, std::size_t open, std::size_t close) {
    std::vector<Span> args;
    std::size_t begin=open+1, separator=seq->links[open].comma;
    for (;;) {
        require(separator<=close,"argument delimiter index invariant");
        args.push_back({seq,begin,separator});
        if(separator==close) break;
        begin=separator+1;separator=seq->links[separator].comma;
    }
    if(args.size()==1 && args[0].begin==args[0].end && m.parameters.empty()) args.clear();
    if(m.variadic) {
        require(args.size()>=m.parameters.size(),"too few variadic arguments");
        args[m.parameters.size()-1].end=close;
        args.resize(m.parameters.size());
    } else require(args.size()==m.parameters.size(),"wrong macro argument count");
    metrics_.argument_spans+=args.size();
    return args;
}
void MacroEngine::substitute(const Macro& m, const PPItem& head, std::uint32_t replacement_paint, const std::vector<Span>& args, const std::vector<std::vector<PPItem>>& expanded, std::vector<PPItem>& out) {
    const std::uint32_t paint_id = add_paint(replacement_paint,m.name);
    const std::uint32_t parameter_paint = add_paint(head.paint,m.name);
    bool join = false;
    // One bulk scratch buffer per substitution, not one allocation per token.
    std::vector<PPItem> part;
    for (std::size_t i=0;i<m.replacement.size();++i) {
        const auto& r = m.replacement[i];
        if (paste(r.item)) { join = true; continue; }
        part.clear();
        if (m.function && hash(r.item)) {
            const auto& raw = args[m.replacement[++i].parameter];
            std::string s = "\"";
            for (std::size_t j=raw.begin;j<raw.end;++j) {
                const auto& token=raw.sequence->items[j];
                if (j>raw.begin && token.space) s += ' ';
                const bool literal = token.token.kind==TokenKind::string || token.token.kind==TokenKind::ud_string || token.token.kind==TokenKind::character || token.token.kind==TokenKind::ud_character;
                for (char c : token.text) { if (literal && (c=='\\' || c=='\"')) s += '\\'; s += c; }
            }
            s += '"'; part.push_back(synthetic(s,head));
        } else if (r.parameter>=0) {
            const std::size_t p = r.parameter;
            const bool comma_extension = m.variadic && p+1==m.parameters.size() && join && !out.empty() && punctuation(out.back(),",");
            if (comma_extension) {
                join=false;
                if(args[p].begin==args[p].end) {out.pop_back(); continue;}
            }
            const bool raw = join || (i+1<m.replacement.size() && paste(m.replacement[i+1].item));
            if(raw) part.assign(args[p].sequence->items.begin()+args[p].begin,args[p].sequence->items.begin()+args[p].end);
            else part=expanded[p];
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
            if (r.parameter<0) inherit_origin(p,head);
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

struct MacroEngine::Rescan::State {
    struct Invocation {
        std::uint32_t macro, replacement_paint;
        PPItem head;
        std::vector<Span> raw;
        std::vector<std::vector<PPItem>> expanded;
        std::size_t next_parameter=0, current_parameter=0;
    };
    struct Frame {
        std::vector<Span> chunks;
        std::vector<PPItem> immediate;
        std::vector<PPItem> output;
        Invocation invocation;
        bool invoking=false;
    };
    std::vector<Frame> frames;
    State() {frames.emplace_back();}
};
MacroEngine::Rescan::Rescan() : state_(new State()) {}
MacroEngine::Rescan::~Rescan() {}
bool MacroEngine::next(PPItem& result, Rescan& rescan, const Pull& source, const Builtin& builtin) {
    auto& frames=rescan.state_->frames;
    using Invocation=Rescan::State::Invocation;
    for (;;) {
        auto& frame=frames.back();
        if(frame.invoking) {
            auto& job=frame.invocation;
            const Macro& macro=definitions_[job.macro];
            if(job.next_parameter<macro.prescan_parameters.size()) {
                std::size_t parameter=macro.prescan_parameters[job.next_parameter++];
                job.current_parameter=parameter;
                Span span=job.raw[parameter];
                frames.emplace_back(); frames.back().chunks.push_back(std::move(span));
                metrics_.max_tasks=std::max(metrics_.max_tasks,frames.size());
                continue;
            }
            std::vector<PPItem> replacement;
            substitute(macro,job.head,job.replacement_paint,job.raw,job.expanded,replacement);
            frame.invocation=Invocation(); frame.invoking=false;
            if(!replacement.empty()) {
                auto seq=sequence(std::move(replacement));
                metrics_.max_pending=std::max(metrics_.max_pending,seq->items.size());
                frame.chunks.push_back({seq,0,seq->items.size()});
            }
            continue;
        }
        std::shared_ptr<const Sequence> pulled_sequence;
        std::size_t pulled_index=0;
        auto pull=[&](PPItem& p) {
            if(!frame.immediate.empty()) {
                pulled_sequence.reset(); p=std::move(frame.immediate.back());frame.immediate.pop_back();return true;
            }
            while(!frame.chunks.empty() && frame.chunks.back().begin==frame.chunks.back().end) frame.chunks.pop_back();
            if(!frame.chunks.empty()) {
                auto& chunk=frame.chunks.back();pulled_sequence=chunk.sequence;pulled_index=chunk.begin;
                p=chunk.sequence->items[chunk.begin++]; return true;
            }
            pulled_sequence.reset();
            return frames.size()==1 && source(p);
        };
        auto unread=[&](PPItem p) {
            if(frame.immediate.empty() && pulled_sequence && !frame.chunks.empty() && frame.chunks.back().sequence==pulled_sequence && frame.chunks.back().begin==pulled_index+1) --frame.chunks.back().begin;
            else frame.immediate.push_back(std::move(p));
        };
        auto emit=[&](PPItem p) {if(frames.size()==1) {result=std::move(p);return true;}frame.output.push_back(std::move(p));return false;};
        PPItem p;
        if(!pull(p)) {
            if(frames.size()==1) return false;
            auto output=std::move(frame.output);frames.pop_back();
            auto& parent=frames.back().invocation;
            parent.expanded[parent.current_parameter]=std::move(output);
            continue;
        }
        ++metrics_.expanded;
        if(p.token.kind!=TokenKind::identifier || p.blocked) {if(emit(std::move(p)))return true;continue;}
        require(p.token.identifier!=va_,"__VA_ARGS__ outside replacement list");
        ++metrics_.lookups;
        if(builtin && builtin(p)) {if(emit(std::move(p)))return true;continue;}
        IdentifierId id=p.token.identifier;
        if(!defined(id)) {if(emit(std::move(p)))return true;continue;}
        const std::uint32_t macro_index=bindings_[id]-1;
        const Macro& macro=definitions_[macro_index];
        // Object-like single-token aliases need neither delimiter indexing nor
        // an argument task. Keep this ordinary rescan work on a compact stack.
        if(!macro.function && macro.replacement.size()<=1) {
            if(painted(p.paint,id)) {p.blocked=true;if(emit(std::move(p)))return true;continue;}
            ++metrics_.invocations;
            if(!macro.replacement.empty()) {
                PPItem replacement=macro.replacement[0].item;
                replacement.paint=add_paint(p.paint,id);
                inherit_origin(replacement,p); replacement.space=p.space;
                frame.immediate.push_back(std::move(replacement));
                metrics_.max_pending=std::max(metrics_.max_pending,frame.immediate.size());
            }
            continue;
        }
        Invocation job;
        job.macro=macro_index; job.head=p;job.replacement_paint=p.paint;
        if(macro.function) {
            PPItem open;
            if(!pull(open)) {if(emit(std::move(p)))return true;continue;}
            if(!punctuation(open,"(") || painted(p.paint,id)) {
                if(punctuation(open,"(")) p.blocked=true;
                unread(std::move(open));if(emit(std::move(p)))return true;continue;
            }
            std::shared_ptr<const Sequence> seq;
            std::size_t start=0,close=0;
            if(frame.immediate.empty() && pulled_sequence && !frame.chunks.empty() && frame.chunks.back().sequence==pulled_sequence && pulled_sequence->links[pulled_index].close<frame.chunks.back().end) {
                seq=pulled_sequence;start=pulled_index;close=seq->links[start].close;
                frame.chunks.back().begin=close+1;
            } else {
                // Cross-chunk/source invocation: capture once, then all nested
                // prescans use indexed spans over this one compact sequence.
                std::vector<PPItem> raw;raw.push_back(std::move(open));
                std::size_t depth=0; PPItem t;
                for (;;) {
                    require(pull(t),"unterminated macro invocation");
                    bool end=!depth && punctuation(t,")");
                    if(!end) {if(punctuation(t,"("))++depth;else if(punctuation(t,")"))--depth;++metrics_.argument_tokens;}
                    raw.push_back(std::move(t)); if(end)break;
                }
                seq=sequence(std::move(raw));close=seq->items.size()-1;
            }
            job.replacement_paint=intersect_paint(p.paint,seq->items[close].paint);
            job.raw=arguments(macro,seq,start,close);
        } else if(painted(p.paint,id)) {p.blocked=true;if(emit(std::move(p)))return true;continue;}
        ++metrics_.invocations;
        job.expanded.resize(job.raw.size());
        frame.invocation=std::move(job); frame.invoking=true;
    }
}
std::vector<PPItem> MacroEngine::expand_owned(std::vector<PPItem> input, const Builtin& builtin) {
    // A directive operand is already an owned flat sequence. Stream it directly;
    // only a demanded function invocation needs retained delimiter links.
    // Avoid indexing/copying every ordinary controlling-expression token.
    Rescan rescan; std::size_t index=0;
    std::vector<PPItem> out; out.reserve(input.size()); PPItem p;
    Pull pull=[&](PPItem& item) {
        if(index==input.size()) return false;
        item=std::move(input[index++]); return true;
    };
    while(next(p,rescan,pull,builtin)) out.push_back(std::move(p));
    return out;
}
std::vector<PPItem> MacroEngine::expand(const std::vector<PPItem>& input, const Builtin& builtin) {
    return expand_owned(input,builtin);
}
}
