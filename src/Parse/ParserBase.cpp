
#include "ParserBase.h"			// parser::ParserBase

#include "Alphabet.h"			// parser::Alphabet
#include "OperatorParsing.h"	// is_binary_operator, is_unary_operator, get_operator_type...
#include "StaticString.h"		// parser::StaticString

#include <array>	// std::array

namespace keywords
{
	static const parse::StaticString s_var{ "var" };
	static const parse::StaticString s_true{ "true" };
	static const parse::StaticString s_false{ "false" };
	static const parse::StaticString s_if{ "if" };
	static const parse::StaticString s_else{ "else" };
	static const parse::StaticString s_while{ "while" };
	static const parse::StaticString s_for{ "for" };

	bool is_keyword(const char * str)
	{
		static const std::array<parse::StaticString, 7u> all_keywords =
		{
			s_var,
			s_true, s_false,
			s_if, s_else,
			s_while, s_for
		};

		for (const auto & keyword : all_keywords)
		{
			if (keyword.same_beggining(str))
			{
				const auto last_char = str[keyword.size()];
				if (parse::is_letter(last_char) || parse::is_number(last_char) || last_char == '_')
					return false;

				return true;
			}
		}

		return false;
	}
}

namespace parse
{
	bool is_number(char c)
	{
		static const Alphabet alphabet{ { '0', '9' } };
		return alphabet.contains(c);
	}
	bool is_letter(char c)
	{
		static const Alphabet alphabet{ { 'a', 'z' }, { 'A', 'Z' } };
		return alphabet.contains(c);
	}
	bool is_operator(const char * str)
	{
		// NOTE(Borja): unary operators usually are binary operators also, so most probably
		// here we will only make the first function call
		return is_binary_operator(str) || is_unary_operator(*str);
	}
	bool is_space(char c) { return c == ' ' || c == '\t'; }
	bool is_new_line(char c) { return c == '\n'; }

	template <char C>
	bool is_char(char c) { return c == C; }

	int parse_integer(const char * str)
	{
		return std::atoi(str);
	}
	float parse_floating_point(const char * str)
	{
		return static_cast<float>(std::atof(str));
	}
	NumericValueType get_number_type(const char * str, std::size_t s)
	{
		for (unsigned i = 0; i < s; i++)
		{
			if (!is_number(str[i]) && str[i] == '.')
				return NumericValueType::FLOAT;
		}

		return NumericValueType::INT;
	}
	NumericValueType get_number_type(const char * str)
	{
		std::size_t length = 0;
		while (is_number(str[length]))
			length++;

		if (str[length] == '.' && is_number(str[length + 1]))
			return NumericValueType::FLOAT;

		return NumericValueType::INT;
	}
}

namespace parse
{
	class ParserBase::Identifier
	{
	public:
		Identifier() = default;
		Identifier(const char * str, std::size_t s);

		const char * get_str() const;
		std::size_t get_length() const;

		const char * get_next_token() const;

	private:
		const char * m_str{ nullptr };
		std::size_t m_size{ 0 };
	};
	ParserBase::Identifier::Identifier(const char * str, std::size_t s)
		: m_str{ str }
		, m_size{ s }
	{}
	const char * ParserBase::Identifier::get_str() const { return m_str; }
	std::size_t ParserBase::Identifier::get_length() const { return m_size; }
	const char * ParserBase::Identifier::get_next_token() const
	{
		return advance_while<parse::is_space>(m_str + m_size);
	}

	void ParserBase::parse(const std::string & file_contents)
	{
		set_new_file_contents(file_contents);

		try
		{
			parse_internal();
		}
		catch (...)
		{
			reset();
			throw;
		}
	}

	void ParserBase::eat_spaces()
	{
		advance_while<parse::is_space>(false);
	}
	void ParserBase::eat_new_lines()
	{
		auto prev = m_curr;
		advance_while<parse::is_new_line>(false);
		m_curr_line += (m_curr - prev);
	}
	void ParserBase::eat_all_untill_next_token()
	{
		while (!is_end_of_script() &&
			(is_space() || is_new_line()))
		{
			eat_spaces();
			eat_new_lines();
		}
	}

	void ParserBase::advance(std::size_t n)
	{
		m_curr += n;
		eat_spaces();
	}

	const char * ParserBase::get_current_location() const { return m_curr; }
	char ParserBase::get_current_char() const { return *get_current_location(); }
	std::size_t ParserBase::get_curr_line_num() const { return m_curr_line; }

	ParserBase::Identifier ParserBase::get_identifier(const char * str)
	{
		// our variable names can have letters underscores and numbers,
		// the first value cannot be a number, but at this point we already know it isn't
		const char * var_end = str;
		while (parse::is_letter(*var_end) ||
			parse::is_number(*var_end) ||
			*var_end == '_')
		{
			++var_end;
		}
		return Identifier(str, var_end - str);
	}
	ParserBase::Identifier ParserBase::get_identifier() const
	{
		return get_identifier(get_current_location());
	}

	bool ParserBase::is_char(char c, std::size_t offset) const
	{
		return get_current_location()[offset] == c;
	}
	bool ParserBase::is_letter() const
	{
		return parse::is_letter(get_current_char());
	}
	bool ParserBase::is_semicolon() const
	{
		return get_current_char() == ';';
	}
	bool ParserBase::is_new_line() const
	{
		return parse::is_new_line(get_current_char());
	}
	bool ParserBase::is_space() const
	{
		return parse::is_space(get_current_char());
	}
	bool ParserBase::is_end_of_script() const
	{
		return get_current_char() == '\0';
	}
	bool ParserBase::is_keyword(const StaticString & keyword) const
	{
		const char * const curr_loc = get_current_location();
		const char lchar = curr_loc[keyword.size()];

		// at this point we only care about the fact this word been the input keyword,
		// error handling because the statatement is not well formed will come later
		return keyword.same_beggining(curr_loc) &&
			!parse::is_letter(lchar) &&
			!parse::is_number(lchar);
	}

	bool ParserBase::is_identifier(const char * str)
	{
		const auto ident = get_identifier(str);
		return ident.get_length() > 0;
	}
	bool ParserBase::is_variable(const char * str)
	{
		return parse::is_letter(*str) && !keywords::is_keyword(str) && !is_function_call(str);
	}
	bool ParserBase::is_bool_value(const char * str)
	{
		return keywords::s_true.same_beggining(str) ||
			keywords::s_false.same_beggining(str);
	}
	bool ParserBase::is_function_call(const char * str)
	{
		const auto ident = get_identifier(str);
		return ident.get_length() > 0 && *ident.get_next_token() == '(';
	}

	bool ParserBase::is_operator() const
	{
		return ::parse::is_operator(get_current_location());
	}
	bool ParserBase::is_comment() const
	{
		return is_char('/') && (is_char('/') || is_char('*'));
	}
	bool ParserBase::is_unary_operator() const
	{
		return ::parse::could_be_unary_operator(get_current_char());
	}
	bool ParserBase::is_plus_plus() const
	{
		const char * curr_loc = get_current_location();
		return curr_loc[0] == '+' && curr_loc[1] == '+';
	}
	bool ParserBase::is_minus_minus() const
	{
		const char * curr_loc = get_current_location();
		return curr_loc[0] == '-' && curr_loc[1] == '-';
	}
	bool ParserBase::is_character() const
	{
		return is_char('\'');
	}
	bool ParserBase::is_number() const
	{
		// we need to test first for unary operators becauseif the number 
		// is '-2' is_number() will return false as the current token is '-'
		return is_unary_operator() || ::parse::is_number(get_current_char());
	}
	bool ParserBase::is_identifier() const
	{
		return is_identifier(get_current_location());
	}
	bool ParserBase::is_variable_decl() const
	{
		const char * curr_loc = get_current_location();
		return keywords::s_var.same_beggining(curr_loc) &&
			parse::is_space(curr_loc[keywords::s_var.size()]);
	}
	bool ParserBase::is_variable() const
	{
		return is_variable(get_current_location());
	}
	bool ParserBase::is_keyword() const
	{
		return keywords::is_keyword(get_current_location());
	}
	bool ParserBase::is_bool_value() const
	{
		const char * curr_loc = get_current_location();
		return is_bool_value(curr_loc) ||
			(is_char('!') && is_bool_value(advance_while<parse::is_space>(get_current_location() + 1)));
	}
	bool ParserBase::is_string() const
	{
		return is_char('"');
	}
	bool ParserBase::is_scope() const
	{
		return is_char('{');
	}
	bool ParserBase::is_if() const
	{
		return is_keyword(keywords::s_if);
	}
	bool ParserBase::is_else() const
	{
		return is_keyword(keywords::s_else);
	}
	bool ParserBase::is_while() const
	{
		return is_keyword(keywords::s_while);
	}
	bool ParserBase::is_for() const
	{
		return is_keyword(keywords::s_for);
	}
	bool ParserBase::is_function_call() const
	{
		return is_function_call(get_current_location());
	}

	void ParserBase::error_if(bool b, const char * err) const
	{
		if (b)	throw except::ParseException{ err, get_curr_line_num() };
	}
	void ParserBase::parse_or_error(char c, const char * err)
	{
		error_if(!parse_char(c), err);
	}

	bool ParserBase::parse_char(char c)
	{
		if (is_char(c))
		{
			advance();
			return true;
		}
		return false;
	}
	bool ParserBase::parse_new_line()
	{
		if (parse_char('\n'))
		{
			m_curr_line++;
			return true;
		}

		// if we reached the end of the string is the same as the
		// end of the last line
		return is_char('\0');
	}

	void ParserBase::parse_comment()
	{
		parse_or_error('/', "Trying to parse comment where there is no one.");

		if (parse_char('/'))
		{
			while (!is_char('\n') && !is_end_of_script())
				advance();
		}
		else if (parse_char('*'))
		{
			// advance untill we find the tokens '*' and '/' together
			while (!(parse_char('*') && parse_char('/')))
				advance();
		}
		else
			error_if(true, "Comments need to start with // or /*");

		eat_all_untill_next_token();
	}
	void ParserBase::parse_statement()
	{
		parse_rvalue_statament();

		if (is_operator())
			parse_equation();
	}
	bool ParserBase::parse_post_inc_dec()
	{
		if (is_plus_plus())
		{
			parse_operator_impl(OperatorType::POST_INC);
			advance(2);
			return true;
		}
		else if (is_minus_minus())
		{
			parse_operator_impl(OperatorType::POST_DEC);
			advance(2);
			return true;
		}

		return false;
	}
	void ParserBase::parse_lvalue_statement()
	{
		if (is_comment())		parse_comment();
		else if (is_if())		parse_if();
		else if (is_while())	parse_while();
		else if (is_for())		parse_for();
		else if (is_scope())	parse_scope();
		else if (is_function_call())	parse_global_function_call();
		else if (is_variable_decl())
		{
			// skip "var" keyword
			advance(keywords::s_var.length());
			eat_spaces();

			parse_variable(true);
			parse_assingnment();
		}
		else if (is_variable())
		{
			parse_variable();
			parse_assingnment();
		}
		else	parse_statement();
	}
	void ParserBase::parse_character()
	{
		parse_or_error('\'', "");
		parse_character_impl(get_current_char());
		advance();
		parse_or_error('\'', "");

		eat_all_untill_next_token();
	}
	void ParserBase::parse_number()
	{
		// get a possible unary operator
		OperatorType op = get_operator_type(get_current_location());
		advance_while<::parse::could_be_unary_operator>();

		parse_number_impl();

		// in case the number is like "-2" or "+38" or "~24"
		if (could_be_unary_operator(op))
		{
			if (op == OperatorType::ADD)		op = OperatorType::UNARY_PLUS;
			else if (op == OperatorType::SUB)	op = OperatorType::UNARY_MINUS;
			parse_operator_impl(op);
		}

		// in case is a floating point, parse the dot
		advance_while<parse::is_number>(false);
		parse_char('.');

		// eat spaces here, in previous calls we only care about the number
		advance_while<parse::is_number>();
	}
	void ParserBase::parse_operator()
	{
		const auto op = ::parse::get_operator_type(get_current_location());
		parse_operator_impl(op);

		advance(::parse::get_operator_str(op).size());
	}
	bool ParserBase::parse_rvalue_statament()
	{
		if (is_function_call())		parse_global_function_call();
		else if (parse_variable_extended()) {}
		else if (is_character())	parse_character();
		else if (is_bool_value())	parse_bool_value();
		else if (is_number())		parse_number();
		else if (is_string())		parse_string();
		else if (parse_char('('))
		{
			parse_statement();
			parse_or_error(')', "Expected token ')' while parsing an equation.");
		}
		else if (is_char('['))	parse_vector_decl();
		else
			return false;	// error

		return true;	// correctly parsed
	}
	void ParserBase::parse_equation()
	{
		// number of operations that this equation needs to do
		std::size_t oper_num = 0;

		// parse an operator and what it follows 
		// ex.: X + Y + Z
		// we would have parsed X above and now we will parse "+ Y" in the first
		// iteration and "+ Z" in the second.
		while (is_operator())
		{
			oper_num++;
			parse_operator();

			const bool parsed_something = parse_rvalue_statament();
			error_if(!parsed_something, "Unexpected token");

			if (is_new_line())	break;
		}

		// in case of having an equation like ((2 + 3)) the outter brackets
		// did not produce any operation, so there is nothing to tie
		if (oper_num > 0)
		{
			tie_equation_impl(oper_num);
			eat_spaces();
		}
	}
	void ParserBase::parse_assingnment()
	{
		if (parse_char('='))
		{
			error_if(is_operator() && !is_unary_operator(), "Unexpected operator after operator '='");

			parse_statement();

			tie_assignment_operator_impl();
			eat_all_untill_next_token();
		}
		else if (is_operator())	// case where is a compound assignment (i.e. +=, -=, ...)
		{
			parse_operator();
			parse_statement();
			tie_equation_impl(1);

			eat_all_untill_next_token();
		}
	}
	void ParserBase::parse_variable(bool decl)
	{
		error_if(is_keyword(), "Variable name cannot be a keyword.");
		const auto identifier = get_identifier();
		parse_variable_impl(identifier.get_str(), identifier.get_length(), decl);
		post_parse_variable(identifier);
	}
	bool ParserBase::parse_variable_extended()
	{
		if (is_variable())	// normal variable
		{
			parse_variable();

			// post-increment/decrement
			if (is_plus_plus())
			{
				parse_operator_impl(OperatorType::POST_INC);
				advance(2);
			}
			else if (is_minus_minus())
			{
				parse_operator_impl(OperatorType::POST_DEC);
				advance(2);
			}
			return true;
		}
		else if (is_unary_operator())
		{
			// pre-increment/decrement
			if (is_plus_plus() || is_minus_minus())
			{
				const bool plus_plus = is_plus_plus();
				advance(2);
				parse_variable();
				parse_operator_impl(plus_plus ? OperatorType::PRE_INC : OperatorType::PRE_DEC);
				return true;
			}
			else
			{
				// we will be checking the next tokens whitout advancing the actual
				// current location

				auto op = get_operator_type(get_current_location());
				if (op == OperatorType::ADD)		op = OperatorType::UNARY_PLUS;
				else if (op == OperatorType::SUB)	op = OperatorType::UNARY_MINUS;

				const char * next_token = advance_while<parse::is_space>(get_current_location() + 1);
				const bool fn_call = is_function_call(next_token);
				const bool variable = is_variable(next_token);
				if (fn_call || variable)
				{
					advance(1);

					if (fn_call)		parse_global_function_call();
					else if (variable)	parse_variable_extended();

					parse_operator_impl(op);
					return true;
				}
			}
		}

		return false;
	}
	void ParserBase::parse_bool_value()
	{
		const bool negated = parse_char('!');

		if (keywords::s_true.same_beggining(get_current_location()))
		{
			parse_bool_value_impl(true);
			advance(keywords::s_true.length());
		}
		else
		{
			parse_bool_value_impl(false);
			advance(keywords::s_false.length());
		}
		eat_spaces();

		if (negated)	parse_operator_impl(OperatorType::LOGIC_NOT);
	}
	void ParserBase::parse_string()
	{
		parse_or_error('"', "Expected to find an string (i.e. '\"' token).");

		const char * const str_start = get_current_location();
		std::size_t len = 0;
		while (!parse_char('"'))
		{
			len++;
			m_curr++;

			// TODO(Borja): is there any way to know if the ending quote ends in the same line??
			// for the moment our strings can be closed in different lines.

			error_if(is_end_of_script(), "Not correctly closed string found.");
		}

		parse_string_impl(str_start, len);
	}
	void ParserBase::parse_scope()
	{
		if (is_char('{'))	parse_multi_line_scope();
		else				parse_single_line_scope();
	}
	void ParserBase::parse_single_line_scope()
	{
		parse_lvalue_statement();
		eat_new_lines();
		tie_scope_impl(1);
	}
	void ParserBase::parse_multi_line_scope()
	{
		parse_char('{');
		eat_all_untill_next_token();

		std::size_t statatement_num = 0;
		while (!parse_char('}'))
		{
			statatement_num++;
			parse_lvalue_statement();

			eat_all_untill_next_token();
			error_if(is_end_of_script(), "Reached end of the script before a matching '}' was found");
		}

		tie_scope_impl(statatement_num);
	}
	void ParserBase::parse_if()
	{
		advance(keywords::s_if.size());

		// condition
		parse_or_error('(', "If statement condition must start with '('.");
		parse_statement();
		parse_or_error(')', "If statement condition must end with ')'.");

		// statements to be exexuted if condition evaluates to true
		parse_scope();

		// parse any continuation
		eat_all_untill_next_token();
		if (is_else())
		{
			advance(keywords::s_else.size());

			if (is_if()) parse_if();
			else		 parse_scope();
			tie_if_impl(true);
		}
		else
			tie_if_impl(false);
	}
	void ParserBase::parse_while()
	{
		advance(keywords::s_while.size());

		// condition
		parse_or_error('(', "While statement condition must start with '('.");
		parse_statement();
		parse_or_error(')', "While statement condition must end with ')'.");

		// statements to be exexuted if condition evaluates to true
		parse_scope();

		tie_while_impl();
	}
	void ParserBase::parse_for()
	{
		advance(keywords::s_for.size());

		// we will be checking if there is any statement in each of the blocks 
		// i.e. for ( 'left' ; 'mid' ; 'right') { }
		// for then calling tie_for_impl_impl

		// left statement
		parse_or_error('(', "For statement condition must start with '('.");
		const bool left = !is_char(';');
		if (left)
			parse_lvalue_statement();

		// mid statement (condition)
		parse_or_error(';', "Missing ';' of the For first and second statement separation.");
		const bool mid = !is_char(';');
		if (mid)
			parse_statement();

		// right statement
		parse_or_error(';', "Missing ';' of the For second and third statement separation.");
		const bool right = !is_char(')');
		if (right)
			parse_statement();

		parse_or_error(')', "For statement must end with ')'.");
		eat_all_untill_next_token();
		parse_scope();

		tie_for_impl(left, mid, right);
	}
	void ParserBase::parse_vector_decl()
	{
		parse_char('[');

		const auto init_list_size = parse_statement_list(']');
		tie_vector_decl_impl(init_list_size);
	}
	void ParserBase::parse_vector_access()
	{
		// vector accesess can be concatenated (i.e. v[0][3][45])
		while (parse_char('['))
		{
			parse_statement();
			parse_or_error(']', "Vector access needs to finish with ']'.");
			tie_vector_access_impl();
		}
	}
	void ParserBase::parse_function_call(Identifier & ident, std::size_t & params)
	{
		const auto fn_name = get_identifier();
		advance(fn_name.get_length());
		parse_or_error('(', "Function call parameter list needs to start with an '('");

		ident = fn_name;
		params = parse_statement_list(')');
	}
	void ParserBase::parse_global_function_call()
	{
		Identifier fn_name;
		std::size_t param_num;
		parse_function_call(fn_name, param_num);
		tie_global_function_call_impl(fn_name.get_str(), fn_name.get_length(), param_num);

		parse_vector_access();
		parse_member_access();
	}
	void ParserBase::parse_member_access()
	{
		while (parse_char('.'))
		{
			if (is_function_call())	parse_meber_function_call();
			else if (is_variable())	parse_meber_variable_access();
			else					error_if(true, "Member access need to be an identifier.");

			parse_vector_access();
		}
	}
	void ParserBase::parse_meber_function_call()
	{
		Identifier fn_name;
		std::size_t param_num;
		parse_function_call(fn_name, param_num);
		tie_member_function_call_impl(fn_name.get_str(), fn_name.get_length(), param_num);
	}
	void ParserBase::parse_meber_variable_access()
	{
		const auto identifier = get_identifier();
		parse_member_variable_impl(identifier.get_str(), identifier.get_length());
		post_parse_variable(identifier);
	}
	std::size_t ParserBase::parse_statement_list(char final_char)
	{
		std::size_t elem_num = 0;

		// parse all elements in the initialization list
		while (!parse_char(final_char))
		{
			elem_num++;
			parse_statement();

			if (!parse_char(','))
				error_if(!is_char(final_char), "Statements in vector initialization need to be separated by comas ','.");
		}

		return elem_num;
	}

	void ParserBase::post_parse_variable(const Identifier & variable_name)
	{
		// skip the variable name
		m_curr = variable_name.get_next_token();
		eat_spaces();

		parse_vector_access();
		parse_member_access();
		parse_post_inc_dec();
	}

	void ParserBase::set_new_file_contents(const std::string & file_contents)
	{
		// if this is the first time we are parsing, no need to reset
		if (m_curr)	reset();

		m_file_contents = file_contents;
		m_curr = m_file_contents.c_str();
	}
	void ParserBase::clear_file_contents()
	{
		m_file_contents.clear();
		m_curr = nullptr;
		m_curr_line = 1;
	}
	void ParserBase::parse_internal()
	{
		while (*m_curr != '\0')
		{
			const auto start = m_curr;

			eat_all_untill_next_token();
			parse_lvalue_statement();

			// if nothing was parsed, there must be an error
			error_if(start == m_curr, "Unexpected token: found a token at a place the parser didn't expect it.");
		}
	}
	void ParserBase::reset()
	{
		reset_impl();
	}

}


