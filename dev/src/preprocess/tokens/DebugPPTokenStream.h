// (C) 2013 CPPGM Foundation. All Rights Reserved. www.cppgm.org

#pragma once

#include <array>
#include <string>

#include "preprocess/tokens/IPPTokenStream.h"

struct DebugPPTokenStream : IPPTokenStream
{
	DebugPPTokenStream() = default;
	DebugPPTokenStream(const DebugPPTokenStream&) = delete;
	DebugPPTokenStream& operator=(const DebugPPTokenStream&) = delete;
	~DebugPPTokenStream() { flush(); }
	void emit_whitespace_sequence()
	{
		append("whitespace-sequence 0 \n", 23);
	}

	void emit_new_line()
	{
		append("new-line 0 \n", 12);
	}

	void emit_header_name(const std::string& data)
	{
		write_token("header-name", data);
	}

	void emit_identifier(const std::string& data)
	{
		write_token("identifier", data);
	}

	void emit_pp_number(const std::string& data)
	{
		write_token("pp-number", data);
	}

	void emit_character_literal(const std::string& data)
	{
		write_token("character-literal", data);
	}

	void emit_user_defined_character_literal(const std::string& data)
	{
		write_token("user-defined-character-literal", data);
	}

	void emit_string_literal(const std::string& data)
	{
		write_token("string-literal", data);
	}

	void emit_user_defined_string_literal(const std::string& data)
	{
		write_token("user-defined-string-literal", data);
	}

	void emit_preprocessing_op_or_punc(const std::string& data)
	{
		write_token("preprocessing-op-or-punc", data);
	}

	void emit_non_whitespace_char(const std::string& data)
	{
		write_token("non-whitespace-character", data);
	}

	void emit_eof()
	{
		append("eof\n", 4);
		flush();
	}

private:

	// Bounded rendering scratch, never a token vector or a phase transport. The
	// previous string type parameter allocated once per long token-kind name and
	// formatted ostream sentries dominated lexical workloads in perf profiles.
	std::array<char, 65536> buffer_;
	std::size_t used_ = 0;

	void flush();
	void append(const char* data, std::size_t size);
	void write_token(const char* type, const std::string& data);
};
