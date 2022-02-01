#pragma once

#include "Operators\OperatorType.h"

#include <vector>	// std::vector

namespace parser
{
	enum TokenType
	{
		OPEN_PAREN, CLOSE_PAREN,			// ( )
		OPEN_CURLY_BRACE, CLOSE_CURLY_BRACE,// { }
		OPEN_SQ_BRACE, CLOSE_SQ_BRACE,		// [ ]
		COMA,	// ,

		STRING,	// " "
		NUMBER,
		IDENTIFIER,
		DOT,	// .

		OPERATOR,

		MAX_TYPES
	};

	class Token
	{
	public:
		Token() = default;
		Token(TokenType type, std::size_t line, const char * start, std::size_t count = 1);

		const char * get_start() const;
		std::size_t get_count() const;
		TokenType get_type() const;
		bool is_of_type(TokenType t) const;

		std::string get_as_string() const;

		const char * cbegin() const;
		const char * cend() const;

		union ExtraData
		{
			OperatorType m_op_type;
		} m_extra;

	private:
		const char * m_start{ nullptr };
		std::size_t m_count{ 0 };
		std::size_t m_line{ 0 };
		TokenType m_type{ TokenType::MAX_TYPES };
	};

	class Tokenizer
	{
	public:
		void tokenize(const std::string & source);

		Token & operator[](std::size_t i);
		const Token & operator[](std::size_t i) const;

		std::vector<Token>::const_iterator cbegin() const;
		std::vector<Token>::const_iterator cend() const;

	private:
		void tokenize_impl();
		void push(TokenType t);
		void push(Token t);

		void set_new_source(const std::string & source);

		void advance(std::size_t n = 1);
		bool is_eof() const;
		const char * get_current() const;
		char get_current_char() const;
		char pop_current_char();
		std::size_t get_current_line() const;

		std::string m_source;
		const char * m_curr{ nullptr };
		std::size_t m_curr_line{ 0 };

		std::vector<Token>	m_tokens;

	};
}
