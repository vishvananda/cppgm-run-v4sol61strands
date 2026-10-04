#pragma once
#include <string>
namespace cppgm {
bool identifier_nondigit(int value);
bool identifier_initial(int value);
void append_utf8(std::string& output, int value);
}
