#include "preprocess/lex/lexer.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace cppgm {
namespace {
std::uint64_t hash_spelling(const std::string& spelling) {
    std::uint64_t h = 14695981039346656037ull;
    for (unsigned char c : spelling) { h ^= c; h *= 1099511628211ull; }
    // Mix short, sequential names before masking into a power-of-two table.
    h ^= h >> 33; h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ull;
    return h ^ (h >> 33);
}
}
bool IdentifierSpelling::equals(const std::string& other) const {
    return size == other.size() && std::memcmp(data, other.data(), size) == 0;
}
void IdentifierTable::grow() {
    std::vector<IdentifierId> new_slots(slots_.empty() ? 16 : slots_.size() * 2, 0);
    const std::size_t mask = new_slots.size() - 1;
    for (std::size_t n = 0; n < entries_.size(); ++n) {
        std::size_t slot = entries_[n].hash & mask;
        while (new_slots[slot]) { ++probes_; slot = (slot + 1) & mask; }
        ++probes_;
        new_slots[slot] = static_cast<IdentifierId>(n + 1);
    }
    slots_.swap(new_slots);
}
IdentifierId IdentifierTable::intern(const std::string& spelling) {
    if (slots_.empty()) grow();
    const std::uint64_t hash = hash_spelling(spelling);
    std::size_t mask = slots_.size() - 1;
    std::size_t slot = hash & mask;
    while (slots_[slot]) {
        ++probes_;
        const Entry& entry = entries_[slots_[slot] - 1];
        if (entry.hash == hash && entry.spelling.equals(spelling)) return slots_[slot];
        slot = (slot + 1) & mask;
    }
    ++probes_;
    if (entries_.size() >= std::numeric_limits<IdentifierId>::max())
        throw std::runtime_error("too many identifiers");
    if ((entries_.size() + 1) * 10 > slots_.size() * 7) {
        grow(); mask = slots_.size() - 1; slot = hash & mask;
        while (slots_[slot]) { ++probes_; slot = (slot + 1) & mask; }
        ++probes_;
    }
    // Allocate bulk spelling slabs, including oversized single names. No
    // null terminator is needed: spellings are immutable length-bearing views.
    if (spelling.size() > slab_capacity_ - slab_used_ || slabs_.empty()) {
        slab_capacity_ = spelling.size() > 65536 ? spelling.size() : 65536;
        slabs_.emplace_back(new char[slab_capacity_]); slab_used_ = 0;
    }
    char* data = slabs_.back().get() + slab_used_;
    std::memcpy(data, spelling.data(), spelling.size());
    slab_used_ += spelling.size();
    entries_.push_back(Entry{{data, spelling.size()}, hash});
    IdentifierId id = static_cast<IdentifierId>(entries_.size());
    slots_[slot] = id;
    return id;
}
IdentifierSpelling IdentifierTable::spelling(IdentifierId id) const {
    return entries_.at(id - 1).spelling;
}
} // namespace cppgm
