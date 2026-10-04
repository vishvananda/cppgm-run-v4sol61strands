#include "preprocess/expr/expression.h"
#include <cstdlib>
#include <iostream>
#include <iterator>

bool defined(void* context, cppgm::IdentifierId id) {
    auto s = static_cast<cppgm::IdentifierTable*>(context)->spelling(id);
    return s.size && (static_cast<unsigned char>(s.data[0]) & 1);
}
int main() {
    std::ios::sync_with_stdio(false);
    cppgm::SourceBuffer source(std::string(std::istreambuf_iterator<char>(std::cin), {}));
    cppgm::IdentifierTable ids;
    cppgm::LexerOptions options; options.collect_literal_elements = true; options.convert_empty_character = true;
    cppgm::Lexer lexer(source, ids, options);
    cppgm::PostCursor cursor(lexer, ids, false, true);
    cppgm::ControllingExpression expressions(cursor, ids, defined, &ids);
    cppgm::PPValue value; bool valid;
    std::uint64_t checksum = 0;
    while (expressions.next(value, valid)) {
        if (!valid) return 1;
        checksum = (checksum ^ value.bits ^ value.is_unsigned) * 1099511628211ull;
    }
    std::cerr << "checksum=" << checksum << " lines=" << expressions.metrics().lines
              << " nodes=" << expressions.metrics().nodes << " evaluated=" << expressions.metrics().evaluated
              << " max_nodes=" << expressions.metrics().max_nodes << " max_stack=" << expressions.metrics().max_stack << '\n';
}
