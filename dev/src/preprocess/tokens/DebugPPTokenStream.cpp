// (C) 2013 CPPGM Foundation. All Rights Reserved. www.cppgm.org
#include "preprocess/tokens/DebugPPTokenStream.h"
#include <algorithm>
#include <cstring>
#include <iostream>

void DebugPPTokenStream::flush()
{
    if (used_) std::cout.write(buffer_.data(), used_);
    used_ = 0;
}

void DebugPPTokenStream::append(const char* data, std::size_t size)
{
    while (size) {
        if (used_ == buffer_.size()) flush();
        std::size_t chunk = std::min(size, buffer_.size() - used_);
        std::memcpy(buffer_.data() + used_, data, chunk);
        used_ += chunk; data += chunk; size -= chunk;
    }
}

void DebugPPTokenStream::write_token(const char* type, const std::string& data)
{
    append(type, std::strlen(type));
    append(" ", 1);
    char digits[3 * sizeof(std::size_t)];
    char* end = digits + sizeof(digits);
    char* begin = end;
    std::size_t size = data.size();
    do { *--begin = static_cast<char>('0' + size % 10); size /= 10; } while (size);
    append(begin, end - begin);
    append(" ", 1);
    append(data.data(), data.size());
    append("\n", 1);
}
