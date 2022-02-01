
#include "OperatorParsing.h"

#include <vector>	// std::vector
#include <array>	// std::array

namespace
{
	static const auto & get_operator_strs()
	{
		static const std::array<parse::StaticString, OperatorType::MAX_TYPES> operators =
		{
			"++", "--", "++", "--",
			"", "",	// unary plus and minus, we leave them empty so that they do not conflict with
					// the binary versions that have less precedence
			"!", "~",
			"*", "/", "%",
			"+", "-",
			"<<", ">>",
			"<", "<=", ">", ">=",
			"==", "!=",
			"&",
			"^",
			"|",
			"&&",
			"||",
			"=",
			"+=", "-=", "*=", "/=", "%=",
			"<<=", ">>=",
			"&=", "^=", "|="
		};

		return operators;
	}

	static const auto & get_precedences()
	{
		static const std::array<std::initializer_list<OperatorType>, 15u> precedences =
		{ {
			{ OperatorType::POST_INC, OperatorType::POST_DEC, OperatorType::PRE_INC, OperatorType::PRE_DEC },
			{ OperatorType::UNARY_MINUS, OperatorType::UNARY_PLUS },
			{ OperatorType::LOGIC_NOT, OperatorType::BITWISE_NOT },
			{ OperatorType::MUL, OperatorType::DIV, OperatorType::MOD },
			{ OperatorType::ADD, OperatorType::SUB },
			{ OperatorType::LEFT_SHIFT, OperatorType::RIGHT_SHIFT },
			{ OperatorType::LESS, OperatorType::LESS_EQ, OperatorType::GREATER, OperatorType::GREATER_EQ },
			{ OperatorType::EQEQ, OperatorType::NOT_EQ },
			{ OperatorType::AND },
			{ OperatorType::XOR },
			{ OperatorType::OR },
			{ OperatorType::LOGIC_AND },
			{ OperatorType::LOGIC_OR },
			{ OperatorType::EQ },
			{ OperatorType::ADD_EQ, OperatorType::SUB_EQ, OperatorType::MUL_EQ, OperatorType::DIV_EQ,  OperatorType::MOD_EQ,
			  OperatorType::LEFT_SHIFT_EQ, OperatorType::RIGHT_SHIFT_EQ,
			  OperatorType::AND_EQ, OperatorType::XOR_EQ, OperatorType::OR_EQ
			}
		} };
		return precedences;
	}
}

namespace parse
{
	const StaticString & get_operator_str(OperatorType op)
	{
		return get_operator_strs()[op];
	}

	OperatorType get_operator_type(const char * str, std::size_t * s)
	{
		const auto & operators = get_operator_strs();

		// the match is determined by the largest operator that matches 'str'
		// this is so that we do not parse an '*=' as an '*', in this case the largest one is '*=' 
		std::size_t largest_i = OperatorType::MAX_TYPES;
		std::size_t largest = 0;
		for (std::size_t i = 0; i < operators.size(); ++i)
		{
			if (operators[i].same_beggining(str))
			{
				if (largest < operators[i].size())
				{
					largest_i = i;
					largest = operators[i].size();
				}
			}
		}

		if (s)	*s = operators[largest_i].size();

		return static_cast<OperatorType>(largest_i);
	}
	OperatorType get_operator_type(char c)
	{
		const auto & operators = get_operator_strs();

		for (unsigned i = 0; i < operators.size(); ++i)
		{
			if (operators[i].size() == 1)
			{
				if (operators[i].same_beggining(&c))
					return static_cast<OperatorType>(i);
			}
		}

		return OperatorType::MAX_TYPES;
	}

	bool is_unary_operator(OperatorType op)
	{
		static const std::array<OperatorType, 8u> unary_operators =
		{
			OperatorType::PRE_INC,
			OperatorType::PRE_DEC,
			OperatorType::POST_INC,
			OperatorType::POST_DEC,
			OperatorType::UNARY_MINUS,
			OperatorType::UNARY_PLUS,
			OperatorType::LOGIC_NOT,
			OperatorType::BITWISE_NOT
		};

		const auto it = std::find(unary_operators.begin(), unary_operators.end(), op);
		return it != unary_operators.end();
	}
	bool could_be_unary_operator(OperatorType op)
	{
		return (op == OperatorType::ADD ||
				op == OperatorType::SUB) ||
			is_unary_operator(op);
	}
	bool could_be_unary_operator(const char * str)
	{
		return could_be_unary_operator(get_operator_type(str));
	}
	bool could_be_unary_operator(char c)
	{
		return could_be_unary_operator(get_operator_type(c));
	}
	bool is_unary_operator(const char * str)
	{
		return is_unary_operator(get_operator_type(str));
	}
	bool is_unary_operator(char c)
	{
		return is_unary_operator(get_operator_type(c));
	}
	bool is_binary_operator(OperatorType op)
	{
		return
			op != OperatorType::MAX_TYPES &&
			!is_unary_operator(op);
	}
	bool is_binary_operator(const char * str)
	{
		return is_binary_operator(get_operator_type(str));
	}
	bool is_binary_operator(char c)
	{
		return is_binary_operator(get_operator_type(c));
	}

	bool operator_has_precedence(OperatorType to_check, std::size_t preference)
	{
		const auto & pref = get_precedences()[preference];
		const auto it = std::find(pref.begin(), pref.end(), to_check);
		return it != pref.end();
	}
	std::size_t get_precedence_num()
	{
		return get_precedences().size();
	}
}