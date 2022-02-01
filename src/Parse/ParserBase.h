#pragma once

#include "Forwards.h"
#include "ScriptingBaseException.h"	// except::ScriptingBaseException

#include <string>	// std::string

// utility functions for the parser
namespace parse
{
	bool is_number(char c);
	bool is_letter(char c);
	bool is_operator(const char * str);
	bool is_space(char c);
	bool is_new_line(char c);

	int parse_integer(const char * str);
	float parse_floating_point(const char * str);

	enum class NumericValueType
	{
		NO_NUMBER = 0,
		INT,
		FLOAT
	};
	NumericValueType get_number_type(const char * str, std::size_t s);
	NumericValueType get_number_type(const char * str);
}

namespace except
{
	class ParseException : public ScriptingBaseException
	{
	public:
		explicit ParseException(const char * err, std::size_t line) throw()
			: ScriptingBaseException{ err }
			, m_line{ line }
		{}

		std::size_t get_line() const { return m_line; }

	private:
		const std::size_t m_line;
	};
}

namespace parse
{

	/// \brief	Implements all the logic of parsing an script and provides the interface 
	///			of how to interpret all this information to a base class.
	///			This parser has been designed having in mind an Stack like behavior, 
	///			each time a token is parsed is pushed into the stack, later the N number of 
	///			elsements in the Stack are combined into one and pushed again.
	///			
	///			Although the behavior is Stack like is better to use a container that gives random 
	///			access over the elements (or at least a container that provides access not just to 
	///			the top element) as some operations won't be able to be interpreted just with this 
	///			push pop operations, such as equations due to operator precedences.
	class ParserBase
	{
	public:
		virtual ~ParserBase() = default;

		void parse(const std::string & file_contents);
		void reset();

	protected:
		template <bool(*FN)(char)>
		static const char * advance_while(const char* start)
		{ 
			const char * ret = start;
			while (FN(*ret)) { ++ret; }
			return ret;
		}
		template <bool(*FN)(char)>
		static const char * get_next_token(const char * curr)
		{
			return advance_while<FN>(curr);
		}

	private:
		class Identifier;
		void advance(std::size_t n = 1);

		template <bool(*FN)(char)>
		void advance_while(bool eatspaces = true) 
		{ 
			m_curr = advance_while<FN>(m_curr);
			if (eatspaces) eat_spaces(); 
		}
		template <bool(*FN)(char)>
		const char * get_next_token() { return get_next_token<FN>(m_curr); }

		void eat_spaces();
		void eat_new_lines();
		void eat_all_untill_next_token();

	protected:
		const char * get_current_location() const;
		char get_current_char() const;
		std::size_t get_curr_line_num() const;

		static Identifier get_identifier(const char * str);
		Identifier get_identifier() const;

		bool is_char(char c, std::size_t offset = 0) const;
		bool is_letter() const;
		bool is_semicolon() const;
		bool is_new_line() const;
		bool is_space() const;
		bool is_end_of_script() const;
		bool is_keyword(const StaticString & keyword) const;

		static bool is_variable(const char * str);
		static bool is_bool_value(const char * str);
		static bool is_function_call(const char * str);
		static bool is_identifier(const char * str);

		bool is_comment() const;
		bool is_operator() const;
		bool is_unary_operator() const;
		bool is_plus_plus() const;
		bool is_minus_minus() const;
		bool is_character() const;
		bool is_number() const;
		bool is_identifier() const;
		bool is_variable_decl() const;
		bool is_variable() const;
		bool is_keyword() const;
		bool is_bool_value() const;
		bool is_string() const;
		bool is_scope() const;
		bool is_if() const;
		bool is_else() const;
		bool is_while() const;
		bool is_for() const;
		bool is_function_call() const;

		bool parse_char(char c);
		bool parse_new_line();

	private:
		void error_if(bool b, const char * err) const;
		void parse_or_error(char c, const char * err);

		void parse_comment();
		void parse_statement();
		void parse_lvalue_statement();
		bool parse_rvalue_statament();
		bool parse_post_inc_dec();
		void parse_character();
		void parse_number();
		void parse_operator();
		void parse_equation();
		void parse_assingnment();
		void parse_variable(bool decl = false);
		bool parse_variable_extended();
		void parse_bool_value();
		void parse_string();
		void parse_scope();
		void parse_single_line_scope();
		void parse_multi_line_scope();
		void parse_if();
		void parse_while();
		void parse_for();
		void parse_vector_decl();
		void parse_vector_access();
		void parse_function_call(Identifier & ident, std::size_t & params);
		void parse_global_function_call();
		void parse_member_access();
		void parse_meber_function_call();
		void parse_meber_variable_access();
		std::size_t parse_statement_list(char final_char);

		void post_parse_variable(const Identifier & variable_name);

		/// \note	Parse functions are called when a token has been read from the file contents
		///			and the parser needs to interpret it (i.e. need to create new AST).

		///	\brief	Called when a new file is going to be parsed.
		virtual void reset_impl() = 0;
		///	\brief	Called when the current token is a number, need to call get_current_location
		///			to get the const char * from where to parse.
		virtual void parse_number_impl() = 0;
		/// \brief	Parses a single character
		virtual void parse_character_impl(char c) = 0;
		///	\brief	Called when the current token is an operator.
		virtual void parse_operator_impl(OperatorType op) = 0;
		/// \brief	Called when a variable is encountered.
		///	\param	var_name	Starting position of the variable.
		///	\param	count		Number of characters that need to be red from 'var_name'.
		///	\param	declaration Whether the variable needs to be created or should already exist.
		virtual void parse_variable_impl(const char * var_name, std::size_t count, 
										 bool declaration) = 0;
		/// \brief	Called when a boolean varue (i.e. true or false) has been parsed.
		///	\param	value	The value that the boolean has.
		virtual void parse_bool_value_impl(bool value) = 0;
		/// \brief	Called when an string needs to be parsed.
		///	\param str		Starting point of the string, next character before '"'.
		///	\param count	Number of characters that need to be red from 'str'.
		virtual void parse_string_impl(const char * str, std::size_t count) = 0;

		/// \note	Tie functions are called after one or multiple tokens have been parsed and need
		///			need to be convined together (i.e. an operation can have multiple numbers/variables 
		///			and operators, after parsing all of them we need to combine them into an 
		///			'operation', tie_equation will be called).

		/// \brief	Called after the rhs of a assignment has been parsed, previous call 
		///			should have been to parse_variable_impl, so these two last nodes need
		///			to be tied into one equation.
		virtual void tie_assignment_operator_impl() = 0;
		/// \brief	Once all the operations of an equation are parsed this function
		///			is the one responsible for grouping them.
		///	\param operations The number of operations that need to be processed, ex.:
		///			Equation-> 2 * (1 + 3) - 4
		///			We will process 2, *, 1, +, 3 and then tie_equation(1) will be called,
		///			to tie the '1 + 3' part. Then we will parse - and 4 and tie_equation(2)
		///			will be called to parse the remaining operators * and -.
		virtual void tie_equation_impl(std::size_t operations) = 0;
		///	\brief	Called once all the statements in an scope have been parsed.
		///	\param	statement_num	Number of statements inside the scope (can be zero)
		virtual void tie_scope_impl(std::size_t statement_num) = 0;
		/// \brief	Called after the end of the scope of the if has been parsed.
		///			The last node are the statement(s) and the previous one the condition, in case of
		///			not having an else. If there is an else the last one is the else, 
		///			then statements to be executed if the if is true and then the if condition.
		///	\param	has_else	Whether or not there is an else after the if.	
		virtual void tie_if_impl(bool has_else) = 0;
		/// \brief	Called when the statements of the while have been parsed.
		///			The last node are the statements and the previous one the condition.
		virtual void tie_while_impl() = 0;
		/// \brief	Called when the statements of the for have been parsed.
		///			The last 4 nodes could be (..., left, mid, right, statements).
		///			Where left, mid and right are the parts of the for 'for (left; mid; right)',
		///			This statements could be empty, the parameters determine if there is something 
		///			(true) or if they are empty.
		virtual void tie_for_impl(bool left, bool mid, bool right) = 0;
		/// \brief	Called when something like this happens (var a = [ 1, 2, 4 ]),
		///			Last N nodes would be the number of values inside the initializer list and the 
		///			N + 1 from the back the variable.
		///	\param init_list_size	Number of elements inside the initialization list.
		virtual void tie_vector_decl_impl(std::size_t init_list_size) = 0;
		/// \brief	Called when the operator[] is found (i.e. v[0] or v[i][j][k], 
		///			in the second case three calls to this function would be made).
		virtual void tie_vector_access_impl() = 0;
		virtual void tie_global_function_call_impl(const char * fn_name, std::size_t count, 
												   std::size_t param_num) = 0;
		virtual void tie_member_function_call_impl(const char * fn_name, std::size_t count,
												   std::size_t param_num) = 0;
		virtual void parse_member_variable_impl(const char * fn_name, std::size_t count) = 0;

	private:
		void set_new_file_contents(const std::string & file_contents);
		void clear_file_contents();
		void parse_internal();

		const char * m_curr{ nullptr };
		std::string m_file_contents;

		std::size_t m_curr_line{ 1 };
	};
}
