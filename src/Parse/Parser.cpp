
#include "Parser.h"

#include "Alphabet.h"				// parser::Alphabet
#include "Runtime\OperatorType.h"	// OperatorType
#include "Parse\OperatorParsing.h"	// parser::get_operator_type

namespace parse
{
	std::unique_ptr<ast::ASTNode> Parser::get_root()
	{
		if (m_nodes.empty())
			return ast::make_noop();
		else if (m_nodes.size() == 1)
		{
			auto root = std::move(m_nodes.front());
			m_nodes.clear();
			return root;
		}

		// we have more than one statement, convine them in 
		// an Statements node and return it as the root
		return ast::make_statements(std::move(m_nodes));
	}

	void Parser::reset_impl()
	{
		m_nodes.clear();
	}
	void Parser::parse_character_impl(char c)
	{
		push_node(ast::make_value(c));
	}
	void Parser::parse_number_impl()
	{
		switch (parse::get_number_type(get_current_location()))
		{
			case parse::NumericValueType::INT:
			{
				const int i = parse::parse_integer(get_current_location());
				push_node(ast::make_value(i));
			} break;
			case parse::NumericValueType::FLOAT:
			{
				const float f = parse::parse_floating_point(get_current_location());
				push_node(ast::make_value(f));
			} break;
		}

	}
	void Parser::parse_operator_impl(OperatorType op)
	{
		// unary plus has no effect
		if (op != OperatorType::UNARY_PLUS)
		{
			if (::parse::is_unary_operator(op))
				push_node(ast::make_unary_operator(op, pop_last_node()));
			else
				push_node(ast::make_operator(op));
		}
	}
	void Parser::parse_variable_impl(const char * str, std::size_t count, bool declaration)
	{
		push_node(ast::make_named_variable(std::string{ str, count }, declaration));
	}
	void Parser::parse_bool_value_impl(bool value)
	{
		push_node(ast::make_value(value));
	}
	void Parser::parse_string_impl(const char * str, std::size_t count)
	{
		push_node(ast::make_value(std::string{ str, count }));
	}

	void Parser::tie_equation_impl(std::size_t operations)
	{
		if (m_nodes.size() < (operations * 2 + 1))
			throw std::runtime_error{ "Wrong operation number." };

		// parse operators by precedence, each iteration a group of 
		// operators is parsed, most precedence first
		for (unsigned i = 0;
			 operations > 0 && i < get_precedence_num();
			 ++i)
		{
			operations = tie_equation_precedence(i, operations);
		}
	}
	void Parser::tie_assignment_operator_impl()
	{
		const auto back_it = std::prev(m_nodes.end());

		// convert nodes 'variable = value' into
		//              =
		//			/		\
		//		variable	value
		auto new_op = ast::make_operator(OperatorType::EQ);
		new_op->set_operands(std::move(*std::prev(back_it)),
							 std::move(*back_it));

		// put the '=' node at the end
		m_nodes.pop_back();
		m_nodes.back() = std::move(new_op);
	}
	void Parser::tie_scope_impl(std::size_t statement_num)
	{
		if (statement_num > 0)
			push_node(ast::make_scope(pop_last_nodes(statement_num)));
		else
			push_node(ast::make_noop());
	}
	void Parser::tie_if_impl(bool has_else)
	{
		// we want this calls to happen in this order, 
		// we cannot put them inside the function call
		auto else_ = pop_last_node_if(has_else);
		auto statement = pop_last_node();
		auto condition = pop_last_node();
		push_node(ast::make_if(std::move(condition), std::move(statement), std::move(else_)));
	}
	void Parser::tie_while_impl()
	{
		auto statements = pop_last_node();
		auto condition = pop_last_node();
		push_node(ast::make_while(std::move(condition), std::move(statements)));
	}
	void Parser::tie_for_impl(bool left, bool mid, bool right)
	{
		// the for loop may have empty parts
		// i.e. an infinite for loop has all empty 'for ( ; ; )'
		// i.e. we could have only a condition, so that behaves a while loop 'for ( ; i < 100; )'
		
		auto statements = pop_last_node();
		auto right_node = pop_last_node_if(right);
		auto condition_node = pop_last_node_if(mid);
		auto left_node = pop_last_node_if(left);
		push_node(ast::make_for(std::move(left_node),
								std::move(condition_node),
								std::move(right_node),
								std::move(statements)));
	}
	void Parser::tie_vector_decl_impl(std::size_t init_list_size)
	{
		push_node(ast::make_vector_decl(pop_last_nodes(init_list_size)));
	}
	void Parser::tie_vector_access_impl()
	{
		auto index = pop_last_node();
		push_node(ast::make_vector_access(pop_last_node(), std::move(index)));
	}
	void Parser::tie_global_function_call_impl(const char * fn_name, std::size_t count,
										std::size_t param_num)
	{
		if (param_num > 0)
			push_node(ast::make_global_fn_call({ fn_name, count }, pop_last_nodes(param_num)));
		else
			push_node(ast::make_global_fn_call({ fn_name, count }, {}));
	}
	void Parser::tie_member_function_call_impl(const char * fn_name, std::size_t count,
											   std::size_t param_num)
	{
		if (param_num > 0)
		{
			auto params = pop_last_nodes(param_num);
			auto inst = pop_last_node();
			push_node(ast::make_member_fn_call({ fn_name, count }, std::move(inst), std::move(params)));
		}
		else
			push_node(ast::make_member_fn_call({ fn_name, count }, pop_last_node(), {}));
	}

	std::vector<std::unique_ptr<ast::ASTNode>> Parser::pop_last_nodes(std::size_t n)
	{
		std::vector<std::unique_ptr<ast::ASTNode>> temp;
		temp.reserve(n);

		auto first = std::prev(m_nodes.end(), n);
		std::move(first, m_nodes.end(), std::back_inserter(temp));
		m_nodes.erase(first, m_nodes.end());
		return temp;
	}
	std::unique_ptr<ast::ASTNode> Parser::pop_last_node()
	{
		auto last = std::move(m_nodes.back());
		m_nodes.pop_back();
		return last;
	}
	std::unique_ptr<ast::ASTNode> Parser::pop_last_node_if(bool b)
	{
		if (b)	return pop_last_node();
		return nullptr;
	}

	void Parser::push_node(std::unique_ptr<ast::ASTNode>&& node)
	{
		m_nodes.emplace_back(std::move(node));
	}

	std::size_t Parser::tie_equation_precedence(std::size_t preferece, std::size_t remaining_opers)
	{
		std::size_t parsed_opers = 0;

		const auto beg_it = std::prev(m_nodes.end(), remaining_opers * 2 + 1);
		auto it = std::next(beg_it);
		while (std::next(it) != m_nodes.end())
		{
			// if have parsed some operator at the end there will be empty nodes
			if (*it == nullptr)	break;

			// skip nodes that are not operators,
			// or that are operators that have already been tied
			auto * pOper = dynamic_cast<ast::BinaryOperator *>(it->get());
			if (!pOper || pOper->has_operands()) { ++it; continue; }

			if (operator_has_precedence(pOper->get_operator_type(), preferece))
			{
				parsed_opers++;

				// merge the operation into one node, if we have nodes (A B C D) and 
				// we are processing (A B C), we would merge (A B C) in E and end up 
				// with (E X X D), where X are empty nodes
				pOper->set_operands(std::move(*std::prev(it)),
									std::move(*std::next(it)));
				*std::prev(it) = std::move(*it);

				// move the empty nodes to the end, after this the nodes would be (E D X X)
				std::move(std::next(it, 2), m_nodes.end(), it);
			}
			else
				++it;
		}

		// remove the empty nodes that we may have left at the end
		if (parsed_opers > 0)
		{
			// we remove "parsed_opers * 2" because when we merge an operation 2 nodes are left
			// empty, after this nodes would be (E D)
			m_nodes.erase(std::prev(m_nodes.end(), parsed_opers * 2),
						  m_nodes.end());
		}

		return remaining_opers - parsed_opers;
	}

	void Parser::parse_member_variable_impl(const char * fn_name, std::size_t count)
	{
		push_node(ast::make_member_var_access(
			std::string{ fn_name, count },
			pop_last_node())
		);
	}
}
