#pragma once

#include "ParserBase.h"

#include "Runtime\AST.h"	// namespace ast

namespace parse
{
	class Parser : public ParserBase
	{
	public:
		std::unique_ptr<ast::ASTNode> get_root();

	private:
		void reset_impl() override;
		void parse_character_impl(char c) override;
		void parse_number_impl() override;
		void parse_operator_impl(OperatorType op) override;
		void parse_variable_impl(const char * str, std::size_t count, bool declaration) override;
		void parse_bool_value_impl(bool value) override;
		void parse_string_impl(const char * str, std::size_t count) override;

		void tie_equation_impl(std::size_t operations) override;
		void tie_assignment_operator_impl() override;
		void tie_scope_impl(std::size_t) override;
		void tie_if_impl(bool has_else) override;
		void tie_while_impl() override;
		void tie_for_impl(bool left, bool mid, bool right) override;
		void tie_vector_decl_impl(std::size_t init_list_size) override;
		void tie_vector_access_impl() override;
		void tie_global_function_call_impl(const char * fn_name, std::size_t count,
										   std::size_t param_num) override;
		void tie_member_function_call_impl(const char * fn_name, std::size_t count,
										   std::size_t param_num) override;
		void parse_member_variable_impl(const char * fn_name, std::size_t count) override;

	private:
		std::vector<std::unique_ptr<ast::ASTNode>> pop_last_nodes(std::size_t n);
		std::unique_ptr<ast::ASTNode> pop_last_node();
		std::unique_ptr<ast::ASTNode> pop_last_node_if(bool b);
		void push_node(std::unique_ptr < ast::ASTNode > && node);

		std::size_t tie_equation_precedence(std::size_t precedence,
											std::size_t remaining_opers);

	private:
		std::vector<std::unique_ptr<ast::ASTNode>> m_nodes;
	};
}
