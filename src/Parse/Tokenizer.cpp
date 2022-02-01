
#include "Tokenizer.h"
#include "OperatorParsing.h"

namespace parser
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

namespace parser
{
	#pragma region // Token

	Token::Token(TokenType type, std::size_t line,
				 const char * start, std::size_t count)
		: m_start{ start }
		, m_count{ count }
		, m_line{ line }
		, m_type{ type }
	{}

	const char * Token::get_start() const
	{
		return m_start;
	}
	std::size_t Token::get_count() const
	{
		return m_count;
	}
	TokenType Token::get_type() const
	{
		return m_type;
	}
	bool Token::is_of_type(TokenType t) const
	{
		return get_type() == t;
	}

	std::string Token::get_as_string() const
	{
		return{ get_start(), get_count() };
	}

	const char * Token::cbegin() const
	{
		return get_start();
	}
	const char * Token::cend() const
	{
		return get_start() + get_count();
	}

	#pragma endregion


	void Tokenizer::tokenize(const std::string & source)
	{
		set_new_source(source);
		tokenize_impl();
	}

	Token & Tokenizer::operator[](std::size_t i)
	{
		return m_tokens[i];
	}
	const Token & Tokenizer::operator[](std::size_t i) const
	{
		return m_tokens[i];
	}

	std::vector<Token>::const_iterator Tokenizer::cbegin() const
	{
		return m_tokens.cbegin();
	}
	std::vector<Token>::const_iterator Tokenizer::cend() const
	{
		return m_tokens.cend();
	}

	void Tokenizer::push(TokenType t)
	{
		push({ t, get_current_line(), get_current() });
	}
	void Tokenizer::push(Token t)
	{
		m_tokens.emplace_back(std::move(t));
	}
	void Tokenizer::tokenize_impl()
	{
		while (!is_eof())
		{
			switch (pop_current_char())
			{
				case ' ': break;
				case '(': push(TokenType::OPEN_PAREN); break;
				case ')': push(TokenType::CLOSE_PAREN); break;
				case '{': push(TokenType::OPEN_CURLY_BRACE); break;
				case '}': push(TokenType::CLOSE_CURLY_BRACE); break;
				case '[': push(TokenType::OPEN_SQ_BRACE); break;
				case ']': push(TokenType::CLOSE_SQ_BRACE); break;
				case ',': push(TokenType::COMA); break;
				case '.':  push(TokenType::DOT); break;
				case '"': 
				{
					const char * start = get_current();
					while (pop_current_char() != '"')
						;

					const char * end = get_current();
					push({ 
						TokenType::STRING, get_current_line(), 
						 start, std::size_t(end - start) 
					});
				} break;
				case '/':
				{
					const char c = get_current_char();
					if (c == '*')	// multi line comment
					{
						advance();

						const char * curr = get_current();
						while (curr[0] != '*' && curr[1] != '/')
						{
							advance();
							curr = get_current();
						}
					}
					else if (c == '/')	// single line comment
					{
						while (pop_current_char() != '\n')
							;
					}
				} break;
				default:
				{
					if (is_operator(get_current()))
					{
						std::size_t count = 0;
						auto op_type = get_operator_type(get_current(), &count);
						Token t{ TokenType::OPERATOR, get_current_line(), get_current(), count };
						t.m_extra.m_op_type = op_type;
						push(t);
					}
				} break;
			}
		}
	}

	void Tokenizer::set_new_source(const std::string & source)
	{
		m_source = source;
		m_curr = m_source.c_str();
		m_tokens.clear();
	}

	void Tokenizer::advance(std::size_t n)
	{
		m_curr += n;
	}
	bool Tokenizer::is_eof() const
	{
		return get_current_char() != '/0';
	}
	const char * Tokenizer::get_current() const
	{
		return m_curr;
	}
	char Tokenizer::get_current_char() const
	{
		return *get_current();
	}
	char Tokenizer::pop_current_char()
	{
		const char c = get_current_char();
		advance();
		return c;
	}
	std::size_t Tokenizer::get_current_line() const
	{
		return m_curr_line;
	}
}

