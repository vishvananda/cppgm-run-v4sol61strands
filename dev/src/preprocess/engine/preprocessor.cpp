#include "preprocess/engine/preprocessor.h"
#include "preprocess/expr/expression.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unordered_set>
#include <limits>

namespace cppgm {
namespace {
void require(bool c,const char* m) { if(!c) throw std::runtime_error(m); }
std::string quote(const std::string& s) {
    std::string r="\""; for(char c:s) { if(c=='"' || c=='\\') r+='\\'; r+=c; } return r+'"';
}
LexerOptions options() { LexerOptions o; o.collect_literal_elements=true; o.convert_empty_character=true; return o; }
struct Identity {
    std::uint64_t device,inode;
    bool operator==(const Identity& b) const { return device==b.device && inode==b.inode; }
};
struct IdentityHash { std::size_t operator()(const Identity& v) const { return v.device*0x9e3779b97f4a7c15ULL ^ v.inode; } };
// Structured adapter for a bounded directive operand. No lexical replay.
class OperandSource : public PPSource {
    const std::vector<PPItem>& items_; std::size_t index_=0;
    PPItem eof_;
    const PPItem* current_=&eof_;
public:
    explicit OperandSource(const std::vector<PPItem>& v):items_(v){}
    Token next() override { current_=index_<items_.size()?&items_[index_++]:&eof_; return current_->token; }
    const std::string& spelling() const override { return current_->text; }
    const std::vector<LiteralElement>& literal_elements() const override { return current_->elements; }
    std::size_t literal_end() const override { return current_->literal_end; }
    std::size_t literal_physical_end() const override { return current_->physical_end; }
    SourceLocation literal_suffix_location() const override { return current_->suffix_location; }
};
std::string string_value(const PPItem& p, IdentifierTable& ids) {
    require(p.token.kind==TokenKind::string && !p.text.empty() && p.text[0]=='"',"expected ordinary string literal");
    std::vector<PPItem> v(1,p); OperandSource source(v); PostCursor cursor(source,ids);
    PostToken t=cursor.next(); require(t.kind==PostKind::array && t.width==1,"invalid string literal");
    return std::string(reinterpret_cast<const char*>(t.units),t.elements-1);
}
}
struct Preprocessor::Impl {
    struct Conditional { bool parent, active, taken, seen_else; };
    struct File {
        std::shared_ptr<const SourceBuffer> buffer; Lexer lexer;
        std::uint32_t name, source_id; std::int64_t line_delta=0;
        Identity identity;
        bool start=true, spaced=false;
        std::size_t conditional_base;
        File(std::shared_ptr<const SourceBuffer> source, IdentifierTable& ids, std::uint32_t n, Identity id, std::size_t base)
            :buffer(std::move(source)),lexer(*buffer,ids,options()),name(n),source_id(0),identity(id),conditional_base(base){}
    };
    IdentifierTable& ids;
    PreprocessorMetrics metrics;
    MacroEngine macros;
    std::vector<std::string> names;
    // Path lookup cache is TU-local. Sources are immutable for preprocessing;
    // pragma-once hits reuse inode identity and never reopen/stat the path.
    // IDs are interned paths, not scans of loaded sources.
    std::vector<File*> path_sources;
    std::vector<std::unique_ptr<File>> sources;
    std::vector<File*> files;
    std::vector<Conditional> conditions;
    std::unordered_set<Identity,IdentityHash> once;
    MacroEngine::Rescan pending;
    std::vector<PPItem> directive;
    bool directive_ready=false;
    IdentifierId file_id,line_id,pragma_id,defined_id;
    std::string date,time;
    std::uint64_t counter=0;
    IdentifierId counter_id;
    std::size_t directive_end_line=0;
    Impl(IdentifierTable& identifiers,const std::string& path,const std::string& d,const std::string& t)
        : ids(identifiers),macros(ids,metrics),date(d),time(t) {
        counter_id=ids.intern("__COUNTER__");
        file_id=ids.intern("__FILE__"); line_id=ids.intern("__LINE__"); pragma_id=ids.intern("_Pragma"); defined_id=ids.intern("defined");
        for (const auto& v : std::vector<std::pair<std::string,std::string>>{
            {"__CPPGM__","201303L"},{"__cplusplus","201103L"},{"__STDC_HOSTED__","1"},
            {"__CPPGM_AUTHOR__","\"Student\""},{"__DATE__",quote(date)},{"__TIME__",quote(time)}}) {
            PPItem origin; PPItem name=macros.synthetic(v.first,origin), value=macros.synthetic(v.second,origin); value.space=true;
            macros.define({name,value});
        }
        include_file(path,true);
    }
    bool active() const { return conditions.empty() || conditions.back().active; }
    bool defined(IdentifierId id) const { return id==file_id || id==line_id || id==counter_id || macros.defined(id); }
    static bool query(void* p,IdentifierId id) { return static_cast<Impl*>(p)->defined(id); }
    bool builtin(PPItem& p) {
        if(p.token.identifier==counter_id) p=macros.synthetic(std::to_string(counter++),p);
        else if(p.token.identifier==file_id) p=macros.synthetic(quote(names[p.file]),p);
        else if(p.token.identifier==line_id) p=macros.synthetic(std::to_string(p.line),p);
        else return false;
        return true;
    }
    MacroEngine::Builtin builtins() { return [this](PPItem& p){return builtin(p);}; }
    void include_file(const std::string& path,bool primary=false) {
        IdentifierId path_id=ids.intern(path);
        if(!primary && path_id<path_sources.size() && path_sources[path_id] && once.count(path_sources[path_id]->identity)) return;
        struct stat s; require(stat(path.c_str(),&s)==0,"missing source/include file");
        Identity id={static_cast<std::uint64_t>(s.st_dev),static_cast<std::uint64_t>(s.st_ino)};
        if(!primary && once.count(id)) return;
        std::shared_ptr<const SourceBuffer> buffer;
        if(path_id<path_sources.size() && path_sources[path_id]) buffer=path_sources[path_id]->buffer;
        else {
            std::ifstream in(path,std::ios::binary); require(bool(in),"cannot open source/include");
            std::ostringstream bytes; bytes<<in.rdbuf();
            buffer=std::make_shared<const SourceBuffer>(bytes.str());
            metrics.source_bytes+=buffer->bytes.size();
        }
        ++metrics.files;
        std::uint32_t name=names.size(); names.push_back(path);
        sources.emplace_back(new File(std::move(buffer),ids,name,id,conditions.size()));
        sources.back()->source_id=sources.size()-1;
        files.push_back(sources.back().get());
        if(path_sources.size()<=path_id) path_sources.resize(path_id+1,nullptr);
        path_sources[path_id]=sources.back().get();
    }
    PPItem read(File& f) {
        Token t=f.lexer.next(); ++metrics.source_tokens;
        PPItem p=capture(f.lexer,t); p.file=f.name;
        p.line=static_cast<std::size_t>(static_cast<std::int64_t>(p.line)+f.line_delta);
        p.token.location.line=p.line; p.token.location.file=f.source_id;
        p.suffix_location.file=f.source_id;
        return p;
    }
    // Stop before executing a directive so pending macro expansions use the
    // definitions at their invocation point. Newlines are whitespace in text.
    bool raw(PPItem& p) {
        if(directive_ready || files.empty()) return false;
        File& f=*files.back();
        for (;;) {
            p=read(f);
            if(p.token.kind==TokenKind::eof) return false;
            if(p.token.kind==TokenKind::whitespace) { f.spaced=true; continue; }
            if(p.token.kind==TokenKind::newline) { f.start=true; f.spaced=true; continue; }
            p.space=f.spaced; f.spaced=false;
            if(f.start && (punctuation(p,"#") || punctuation(p,"%:"))) {
                directive.clear(); bool space=false;
                for (;;) {
                    PPItem t=read(f);
                    if(t.token.kind==TokenKind::eof || t.token.kind==TokenKind::newline) {directive_end_line=t.line; break;}
                    if(t.token.kind==TokenKind::whitespace) {space=true;continue;}
                    t.space=space; space=false; directive.push_back(std::move(t));
                }
                f.start=true; f.spaced=true; directive_ready=true; return false;
            }
            f.start=false;
            if(active()) return true;
        }
    }
    bool condition(std::vector<PPItem> v) {
        // Protect operands of defined from ordinary macro expansion.
        for(std::size_t i=0;i<v.size();++i) if(v[i].token.kind==TokenKind::identifier && v[i].token.identifier==defined_id) {
            v[i].blocked=true;
            std::size_t j=i+1; if(j<v.size() && punctuation(v[j],"(")) ++j;
            if(j<v.size()) {
                v[j].blocked=true;
                // PA1 observes alternative operator words as punctuators;
                // defined accepts their identifier spelling in this course.
                if(v[j].token.kind==TokenKind::punctuator && !v[j].text.empty() && v[j].text[0]>='a' && v[j].text[0]<='z') {
                    v[j].token.kind=TokenKind::identifier; v[j].token.identifier=ids.intern(v[j].text);
                }
            }
        }
        for(std::size_t i=0;i<v.size();++i) {
            if(v[i].text!="__has_cpp_attribute") continue;
            const std::size_t begin=i;
            PPItem origin=v[i];
            require(++i<v.size() && punctuation(v[i],"("),"attribute probe requires (");
            std::string name;
            while(++i<v.size() && !punctuation(v[i],")")) name+=v[i].text;
            require(i<v.size(),"unterminated attribute probe");
            bool present=name=="no_unique_address" || name=="__no_unique_address__";
            v[begin]=macros.synthetic(present?"201803L":"0",origin);
            v.erase(v.begin()+begin+1,v.begin()+i+1); i=begin;
        }
        v=macros.expand_owned(std::move(v),builtins());
        OperandSource source(v); PostCursor cursor(source,ids,false,true);
        ControllingExpression expression(cursor,ids,query,this);
        PPValue value; bool valid=false;
        require(expression.next(value,valid) && valid,"invalid controlling expression");
        return value.bits!=0;
    }
    void pragma(const std::vector<PPItem>& v, Identity identity) {
        if(v.size()==1 && v[0].text=="once") once.insert(identity);
    }
    void execute() {
        directive_ready=false;
        if(directive.empty()) return;
        const std::string command=directive[0].text;
        std::vector<PPItem> v=std::move(directive);
        v.erase(v.begin());
        if(command=="if" || command=="ifdef" || command=="ifndef") {
            bool parent=active(), value=false;
            if(parent) {
                if(command=="if") value=condition(std::move(v));
                else { require(v.size()==1 && v[0].token.kind==TokenKind::identifier,"invalid ifdef"); value=defined(v[0].token.identifier); if(command=="ifndef") value=!value; }
            }
            conditions.push_back({parent,parent && value,parent && value,false}); return;
        }
        if(command=="elif" || command=="else" || command=="endif") {
            require(conditions.size()>files.back()->conditional_base,"unmatched conditional directive");
            Conditional& c=conditions.back();
            if(command=="endif") { if(c.parent) require(v.empty(),"extra endif tokens"); conditions.pop_back(); return; }
            require(!c.seen_else,"conditional after else");
            if(command=="else") { if(c.parent) require(v.empty(),"extra else tokens"); c.seen_else=true; c.active=c.parent && !c.taken; c.taken=true; }
            else { c.active=c.parent && !c.taken && condition(std::move(v)); c.taken=c.taken || c.active; }
            return;
        }
        if(!active()) return;
        if(command=="define") { macros.define(v); return; }
        if(command=="undef") { macros.undefine(v); return; }
        if(command=="error") throw std::runtime_error("active #error");
        if(command=="pragma") { pragma(v,files.back()->identity); return; }
        if(command=="include") {
            v=macros.expand_owned(std::move(v),builtins()); require(v.size()==1,"invalid include operand");
            std::string path;
            if(v[0].token.kind==TokenKind::header) path=v[0].text.substr(1,v[0].text.size()-2);
            else path=string_value(v[0],ids);
            const std::string& current=names[files.back()->name]; std::size_t slash=current.rfind('/');
            if(slash!=std::string::npos) {
                std::string rel=current.substr(0,slash+1)+path;
                IdentifierId id=ids.intern(rel);
                if(id<path_sources.size() && path_sources[id] && once.count(path_sources[id]->identity)) return;
                struct stat s; if(stat(rel.c_str(),&s)==0) {include_file(rel);return;}
            }
            include_file(path); return;
        }
        if(command=="line") {
            v=macros.expand_owned(std::move(v),builtins()); require(v.size()==1 || v.size()==2,"invalid line directive");
            OperandSource source(v); PostCursor cursor(source,ids); PostToken number=cursor.next();
            require(number.kind==PostKind::scalar && number.width<=8 && number.type!=FundamentalType::FT_FLOAT && number.type!=FundamentalType::FT_DOUBLE && number.type!=FundamentalType::FT_LONG_DOUBLE,"invalid line number");
            std::uint64_t n=0; for(std::size_t i=0;i<number.width;++i) n|=std::uint64_t(number.scalar[i])<<(8*i);
            require(n && n<=std::numeric_limits<int>::max(),"line number out of range");
            File& f=*files.back();
            const std::int64_t physical=static_cast<std::int64_t>(directive_end_line)-f.line_delta;
            f.line_delta=static_cast<std::int64_t>(n)-physical-1;
            if(v.size()==2) { std::string name=string_value(v[1],ids); f.name=names.size(); names.push_back(std::move(name)); }
            return;
        }
        throw std::runtime_error("unknown preprocessing directive");
    }
    bool expanded(PPItem& p) {
        for (;;) {
            if(macros.next(p,pending,[this](PPItem& t){return raw(t);},builtins())) return true;
            if(directive_ready) {execute();continue;}
            if(files.empty()) return false;
            require(conditions.size()==files.back()->conditional_base,"unterminated conditional group");
            files.pop_back();
        }
    }
    bool next(PPItem& p) {
        while(expanded(p)) {
            if(p.token.kind!=TokenKind::identifier || p.token.identifier!=pragma_id) return true;
            PPItem open,literal,close;
            require(expanded(open) && punctuation(open,"("),"_Pragma requires (");
            require(expanded(literal),"_Pragma requires literal");
            require(expanded(close) && punctuation(close,")"),"_Pragma requires )");
            require(literal.token.kind==TokenKind::string && (literal.text[0]=='"' || (literal.text[0]=='L' && literal.text[1]=='"')),"invalid _Pragma string");
            // Destringizing is not phase-5 decoding: only escaped quotes and
            // backslashes are replaced; wide prefix is removed.
            std::size_t start=literal.text[0]=='L'?2:1;
            std::string text;
            for(std::size_t i=start;i+1<literal.text.size();++i) {
                if(literal.text[i]=='\\' && i+2<literal.text.size() && (literal.text[i+1]=='\\' || literal.text[i+1]=='"')) ++i;
                text+=literal.text[i];
            }
            SourceBuffer source(text); Lexer lexer(source,ids,options()); std::vector<PPItem> v;
            for (;;) {Token t=lexer.next(); if(t.kind==TokenKind::eof)break; if(t.kind!=TokenKind::whitespace && t.kind!=TokenKind::newline)v.push_back(capture(lexer,t));}
            pragma(v,sources[p.token.location.file]->identity);
        }
        return false;
    }
};
Preprocessor::Preprocessor(IdentifierTable& ids,const std::string& path,const std::string& date,const std::string& time)
    : impl_(new Impl(ids,path,date,time)) {}
Preprocessor::~Preprocessor() {}
Token Preprocessor::next() {
    if(!impl_->next(current_)) current_=PPItem();
    return current_.token;
}
const PreprocessorMetrics& Preprocessor::metrics() const {return impl_->metrics;}
const SourceBuffer& Preprocessor::source_buffer(std::uint32_t id) const {return *impl_->sources.at(id)->buffer;}
const std::string& Preprocessor::file_name(std::uint32_t id) const {return impl_->names.at(id);}
}
