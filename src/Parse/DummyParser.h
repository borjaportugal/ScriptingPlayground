
#pragma once

#include "ParserBase.h"

namespace parse
{
	class DummyParser : public ParserBase
	{
	public:
		void reset_impl() override {}
		void parse_number_impl() override {}
		void parse_character_impl(char) override {}
		void parse_operator_impl(OperatorType) override {}
		void tie_equation_impl(std::size_t) override {}
		void parse_string_impl(const char *, std::size_t) override {}

		void parse_variable_impl(const char *, std::size_t, bool) override {}
		void parse_bool_value_impl(bool) override {}
		void tie_assignment_operator_impl() override {}
		void tie_scope_impl(std::size_t) override {}
		void tie_if_impl(bool) override {}
		void tie_while_impl() override {}
		void tie_for_impl(bool, bool, bool) override {}
		void tie_vector_decl_impl(std::size_t) override {}
		void tie_vector_access_impl() override {}
		void tie_global_function_call_impl(const char *, std::size_t, std::size_t) override {}
		void tie_member_function_call_impl(const char *, std::size_t, std::size_t) override {}
		void parse_member_variable_impl(const char *, std::size_t) override {}
	};
}

