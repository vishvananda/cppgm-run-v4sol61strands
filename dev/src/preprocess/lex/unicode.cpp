#include "preprocess/lex/unicode.h"
#include <algorithm>
#include <utility>

namespace cppgm {
namespace {
// C++11 Annex E.1 / E.2 (ranges inherited from PA1 starter).
const std::pair<int, int> allowed[] =
{
	{0xA8,0xA8},
	{0xAA,0xAA},
	{0xAD,0xAD},
	{0xAF,0xAF},
	{0xB2,0xB5},
	{0xB7,0xBA},
	{0xBC,0xBE},
	{0xC0,0xD6},
	{0xD8,0xF6},
	{0xF8,0xFF},
	{0x100,0x167F},
	{0x1681,0x180D},
	{0x180F,0x1FFF},
	{0x200B,0x200D},
	{0x202A,0x202E},
	{0x203F,0x2040},
	{0x2054,0x2054},
	{0x2060,0x206F},
	{0x2070,0x218F},
	{0x2460,0x24FF},
	{0x2776,0x2793},
	{0x2C00,0x2DFF},
	{0x2E80,0x2FFF},
	{0x3004,0x3007},
	{0x3021,0x302F},
	{0x3031,0x303F},
	{0x3040,0xD7FF},
	{0xF900,0xFD3D},
	{0xFD40,0xFDCF},
	{0xFDF0,0xFE44},
	{0xFE47,0xFFFD},
	{0x10000,0x1FFFD},
	{0x20000,0x2FFFD},
	{0x30000,0x3FFFD},
	{0x40000,0x4FFFD},
	{0x50000,0x5FFFD},
	{0x60000,0x6FFFD},
	{0x70000,0x7FFFD},
	{0x80000,0x8FFFD},
	{0x90000,0x9FFFD},
	{0xA0000,0xAFFFD},
	{0xB0000,0xBFFFD},
	{0xC0000,0xCFFFD},
	{0xD0000,0xDFFFD},
	{0xE0000,0xEFFFD}
};

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const std::pair<int, int> not_initial[] =
{
	{0x300,0x36F},
	{0x1DC0,0x1DFF},
	{0x20D0,0x20FF},
	{0xFE20,0xFE2F}
};

template<std::size_t N>
bool in_ranges(int c, const std::pair<int, int> (&ranges)[N]) {
    const auto it = std::upper_bound(ranges, ranges + N, c,
        [](int value, const std::pair<int, int>& range) { return value < range.first; });
    return it != ranges && c <= (it - 1)->second;
}
}
bool identifier_nondigit(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
        || (c >= 0xA8 && in_ranges(c, allowed));
}
bool identifier_initial(int c) {
    return identifier_nondigit(c) && !(c >= 0x300 && in_ranges(c, not_initial));
}
void append_utf8(std::string& s, int c) {
    if (c < 0x80) s += static_cast<char>(c);
    else if (c < 0x800) {
        s += static_cast<char>(0xC0 | (c >> 6));
        s += static_cast<char>(0x80 | (c & 63));
    } else if (c < 0x10000) {
        s += static_cast<char>(0xE0 | (c >> 12));
        s += static_cast<char>(0x80 | ((c >> 6) & 63));
        s += static_cast<char>(0x80 | (c & 63));
    } else {
        s += static_cast<char>(0xF0 | (c >> 18));
        s += static_cast<char>(0x80 | ((c >> 12) & 63));
        s += static_cast<char>(0x80 | ((c >> 6) & 63));
        s += static_cast<char>(0x80 | (c & 63));
    }
}
} // namespace cppgm
