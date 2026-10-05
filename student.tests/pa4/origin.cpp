// Structured PA4 provenance controls: dumps do not observe physical identities.
#include "preprocess/engine/preprocessor.h"
#include "preprocess/post/cursor.h"
#include <cassert>
#include <fstream>
#include <cstdio>
#include <string>
int main(int argc,char** argv) {
    assert(argc==2); const std::string path=argv[1];
    const std::string text=
        "#define OP \"\"_suffix\n#define CAT(a,b) a##b\n"
        "#define ID(x) x\n#define STR(x) #x\n#line 90 \"virtual.cc\"\n"
        "operator OP\noperator CAT(\"\",_suffix)\noperator ID(\"\"_suffix)\n"
        "STR(word);\nID(\"a\") \"b\"\n";
    {std::ofstream out(path);out<<text;}
    cppgm::IdentifierTable ids;
    cppgm::Preprocessor pp(ids,path,"Oct  5 2026","00:00:00");
    cppgm::PostCursor cursor(pp,ids,true);
    auto check=[&](const cppgm::PostToken& t,const std::string& origin,std::size_t line) {
        const auto& bytes=pp.source_buffer(t.location.file).bytes;
        assert(t.range.begin<=t.range.end && t.range.end<=bytes.size());
        assert(bytes.substr(t.range.begin,t.range.end-t.range.begin)==origin);
        assert(t.location.offset==t.range.begin && t.location.line==line);
        assert(pp.file_name(t.location.presumed_file)=="virtual.cc");
    };
    for(unsigned i=0;i<3;++i) {
        auto op=cursor.next();check(op,"operator",90+i);
        auto literal=cursor.next();assert(literal.kind==cppgm::PostKind::array);
        const std::string origin=i==0?"OP":i==1?"CAT":"\"\"";
        check(literal,origin,90+i);
        auto suffix=cursor.next();assert(suffix.kind==cppgm::PostKind::identifier);
        check(suffix,i==2?"_suffix":origin,90+i);
        assert(ids.spelling(suffix.identifier).equals("_suffix"));
    }
    auto stringized=cursor.next();check(stringized,"STR",93);
    assert(stringized.kind==cppgm::PostKind::array && stringized.elements==5);
    assert(cursor.next().kind==cppgm::PostKind::simple);
    auto concatenated=cursor.next();assert(concatenated.kind==cppgm::PostKind::array);
    check(concatenated,"\"a\") \"b\"",94);
    assert(concatenated.elements==3 && concatenated.units[0]=='a' && concatenated.units[1]=='b');
    assert(cursor.next().kind==cppgm::PostKind::eof);
    assert(pp.source_buffer(stringized.location.file).bytes==text);
    std::remove(path.c_str());
    // Adjacent literals from different source buffers must not acquire a
    // synthetic range using the second file's offset in the first file.
    const std::string header=path+".hh";
    {std::ofstream out(header);out<<"\"a\"\n";}
    {std::ofstream out(path);out<<"#include \""<<header<<"\"\n\"b\"\n";}
    cppgm::Preprocessor included(ids,path,"Oct  5 2026","00:00:00");
    cppgm::PostCursor sequence(included,ids);
    auto joined=sequence.next();
    assert(joined.kind==cppgm::PostKind::array && joined.elements==3);
    assert(joined.units[0]=='a' && joined.units[1]=='b');
    assert(joined.range.begin==0 && joined.range.end==3);
    assert(included.source_buffer(joined.location.file).bytes=="\"a\"\n");
    assert(sequence.next().kind==cppgm::PostKind::eof);
    std::remove(path.c_str()); std::remove(header.c_str());
}
