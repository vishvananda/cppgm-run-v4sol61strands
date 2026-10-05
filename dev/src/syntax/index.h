#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>
namespace cppgm {
// TU/scope-owned dense entries with a flat open-addressed ID index. No entry
// allocation, rendered spelling key or table sized to all identifiers in a TU.
// Entry references survive lookup, but not insertion; callers retain IDs.
template<class Value> class SyntaxIndex {
    using Entry = std::pair<std::uint32_t,Value>;
    std::vector<Entry> entries_;
    std::vector<std::uint32_t> slots_; // entry index + 1, zero denotes vacancy
    static std::uint32_t hash(std::uint32_t key) {
        key ^= key >> 16; key *= 0x7feb352dU;
        key ^= key >> 15; key *= 0x846ca68bU;
        return key ^ (key >> 16);
    }
    std::size_t slot(std::uint32_t key) const {
        auto i=hash(key)&(slots_.size()-1);
        while (slots_[i] && entries_[slots_[i]-1].first!=key)
            i=(i+1)&(slots_.size()-1);
        return i;
    }
    void grow() {
        slots_.assign(slots_.empty() ? 4 : slots_.size()*2,0);
        for (std::size_t i=0;i<entries_.size();++i)
            slots_[slot(entries_[i].first)]=i+1;
    }
public:
    Entry* find(std::uint32_t key) {
        if (slots_.empty()) return nullptr;
        auto index=slots_[slot(key)];
        return index ? &entries_[index-1] : nullptr;
    }
    const Entry* find(std::uint32_t key) const {
        if (slots_.empty()) return nullptr;
        auto index=slots_[slot(key)];
        return index ? &entries_[index-1] : nullptr;
    }
    std::pair<Value*,bool> insert(std::uint32_t key, const Value& value=Value()) {
        if (auto old=find(key)) return {&old->second,false};
        if (slots_.empty() || (entries_.size()+1)*4>slots_.size()*3) grow();
        auto i=slot(key);
        entries_.emplace_back(key,value); slots_[i]=entries_.size();
        return {&entries_.back().second,true};
    }
    Value& operator[](std::uint32_t key) { return *insert(key).first; }
    const Value& at(std::uint32_t key) const {
        auto entry=find(key);
        if (!entry) throw std::out_of_range("syntax identity");
        return entry->second;
    }
    typename std::vector<Entry>::const_iterator begin() const { return entries_.begin(); }
    typename std::vector<Entry>::const_iterator end() const { return entries_.end(); }
};
}
