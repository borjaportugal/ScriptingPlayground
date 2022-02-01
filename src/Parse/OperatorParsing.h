#pragma once

#include "Runtime\OperatorType.h"
#include "Parse\StaticString.h"

namespace parse
{
	const StaticString & get_operator_str(OperatorType op);
	OperatorType get_operator_type(const char * str, std::size_t * s = nullptr);
	OperatorType get_operator_type(char c);

	bool is_unary_operator(OperatorType op);
	bool is_unary_operator(const char * str);
	bool is_unary_operator(char c);
	bool could_be_unary_operator(OperatorType op);
	bool could_be_unary_operator(const char * str);
	bool could_be_unary_operator(char c);

	bool is_binary_operator(OperatorType op);
	bool is_binary_operator(const char * str);
	bool is_binary_operator(char c);

	bool operator_has_precedence(OperatorType to_check, std::size_t preference);
	std::size_t get_precedence_num();
}


