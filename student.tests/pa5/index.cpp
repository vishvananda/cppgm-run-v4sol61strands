#include "syntax/index.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_map>
int main() {
    cppgm::SyntaxIndex<std::uint32_t> index;
    std::unordered_map<std::uint32_t,std::uint32_t> oracle;
    std::uint32_t state=1;
    for (unsigned i=0;i<200000;++i) {
        state=state*1664525U+1013904223U;
        auto key=i%7 ? state : i%100;
        auto found=index.find(key);
        assert(bool(found)==bool(oracle.count(key)));
        if (found) assert(found->second==oracle.at(key));
        index[key]=i; oracle[key]=i;
    }
    index[0]=42; oracle[0]=42;
    index[UINT32_MAX]=123; oracle[UINT32_MAX]=123;
    assert(!index.insert(0,11).second && index.at(0)==42);
    for (const auto& entry:oracle) assert(index.at(entry.first)==entry.second);
    unsigned count=0;
    for (const auto& entry:index) {++count;assert(oracle.at(entry.first)==entry.second);}
    assert(count==oracle.size());
    std::cout << "PA5 compact scope index passed: " << count << " keys\n";
}
