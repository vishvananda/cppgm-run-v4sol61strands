#include "preprocess/engine/preprocessor.h"
#include "preprocess/post/cursor.h"
#include <fstream>
#include <cassert>
#include <cstdio>
int main(int argc,char** argv) {
    assert(argc==2);std::string path=argv[1];
    {std::ofstream out(path);out<<"#define VALUE 23\n#line 80 \"virtual.cc\"\nVALUE suffix\n\"x\"\n";}
    cppgm::IdentifierTable ids;
    cppgm::Preprocessor pp(ids,path,"Oct  5 2026","00:00:00");
    cppgm::PostCursor cursor(pp,ids,true);
    auto number=cursor.next();
    assert(number.kind==cppgm::PostKind::scalar && number.scalar[0]==23);
    assert(number.location.line==80);
    assert(pp.file_name(number.location.presumed_file)=="virtual.cc");
    const auto& bytes=pp.source_buffer(number.location.file).bytes;
    assert(bytes.substr(number.range.begin,number.range.end-number.range.begin)=="VALUE");
    auto suffix=cursor.next();assert(suffix.kind==cppgm::PostKind::identifier);
    assert(pp.source_buffer(suffix.location.file).bytes.substr(suffix.range.begin,suffix.range.end-suffix.range.begin)=="suffix");
    auto string=cursor.next();assert(string.kind==cppgm::PostKind::array && string.units[0]=='x');
    assert(cursor.next().kind==cppgm::PostKind::eof);
    // Lexical frames have been popped; immutable source bytes remain available
    // until the TU owner dies, rather than hanging on to completed lexers.
    assert(pp.source_buffer(number.location.file).bytes==bytes);
    std::remove(path.c_str());
}
