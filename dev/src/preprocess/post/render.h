#pragma once
#include <iosfwd>
#include "preprocess/post/types.h"
namespace cppgm {
void render_posttoken(std::ostream& out, const PostToken& token, const IdentifierTable& identifiers);
}
