#include "preprocess/tokens/DebugPPTokenStream.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int main() {
    std::ostringstream sink;
    std::streambuf* previous = std::cout.rdbuf(sink.rdbuf());
    std::string large(200000, 'x');
    large[65535] = '\0'; large[65536] = '\n';
    std::string expected;
    {
        DebugPPTokenStream output;
        output.emit_identifier(large);
        output.emit_string_literal("");
        output.emit_eof();
        expected = "identifier 200000 " + large + "\nstring-literal 0 \neof\n";
        assert(sink.str() == expected); // eof flushes before destruction.
    }
    {
        DebugPPTokenStream output;
        for (unsigned n = 0; n < 20000; ++n) {
            output.emit_whitespace_sequence();
            output.emit_new_line();
            expected += "whitespace-sequence 0 \nnew-line 0 \n";
        }
        // No EOF on rejected input: destruction flushes prior valid records.
    }
    assert(sink.str() == expected);
    try {
        DebugPPTokenStream output;
        output.emit_identifier("before-rejection");
        throw std::runtime_error("rejected input");
    } catch (const std::runtime_error&) {
        expected += "identifier 16 before-rejection\n";
        assert(sink.str() == expected); // exception unwind preserves valid output.
    }
    std::cout.rdbuf(previous);
    std::cout << "bounded renderer / binary spelling / EOF and unwind flush controls passed\n";
}
