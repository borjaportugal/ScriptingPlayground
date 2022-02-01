
#include "gmock\gmock.h"
using namespace testing;

#include "Parse\Alphabet.h"				// Alphabet
#include "Parse\ParserBase.h"			// 
#include "Parse\DummyParser.h"
#include "Parse\OperatorParsing.h"
#include "Runtime\OperatorType.h"
using namespace parse;

class DummyParserMock : public parse::DummyParser {};

#pragma region // Utility functions test

TEST(CharacterRecognition, parser_can_identify_numbers)
{
	ASSERT_TRUE(is_number('3'));
	ASSERT_FALSE(is_number('d'));
	ASSERT_FALSE(is_number('|'));
	ASSERT_FALSE(is_number('('));
}
TEST(CharacterRecognition, parser_can_identify_letters)
{
	ASSERT_TRUE(is_letter('f'));
	ASSERT_TRUE(is_letter('G'));
	ASSERT_TRUE(is_letter('d'));
	ASSERT_TRUE(is_letter('A'));
	ASSERT_TRUE(is_letter('Z'));
	ASSERT_TRUE(is_letter('a'));
	ASSERT_TRUE(is_letter('z'));
	ASSERT_FALSE(is_letter('|'));
	ASSERT_FALSE(is_letter('('));
}
TEST(CharacterRecognition, parser_can_identify_operators)
{
	ASSERT_TRUE(is_operator("+"));
	ASSERT_TRUE(is_operator("-"));
	ASSERT_TRUE(is_operator("*"));
	ASSERT_TRUE(is_operator("/"));
	ASSERT_TRUE(is_operator("%"));
}
TEST(CharacterRecognition, parser_can_identify_unary_operators)
{
	ASSERT_TRUE(could_be_unary_operator('+'));
	ASSERT_TRUE(could_be_unary_operator('-'));
	ASSERT_FALSE(is_unary_operator('+'));
	ASSERT_FALSE(is_unary_operator('-'));
}

TEST(NumberParsingTest, parser_can_identify_integer_values)
{
	ASSERT_EQ(get_number_type("126"), NumericValueType::INT);
}
TEST(NumberParsingTest, parser_can_identify_floating_point_values)
{
	ASSERT_EQ(get_number_type("2634.86"), NumericValueType::FLOAT);
	ASSERT_EQ(get_number_type("0.24600"), NumericValueType::FLOAT);
	ASSERT_EQ(get_number_type("0010101.24"), NumericValueType::FLOAT);
}
TEST(NumberParsingTest, parser_can_parse_integer_values)
{
	ASSERT_EQ(parse_integer("126"), 126);
	ASSERT_EQ(parse_integer("4289"), 4289);
	ASSERT_EQ(parse_integer("001260"), 1260);
}
TEST(NumberParsingTest, parser_can_parse_floating_point_values)
{
	ASSERT_EQ(parse_floating_point("126.386"), 126.386f);
}

#pragma endregion

#pragma region // Value parsing test

struct Parser_ValueMock : public DummyParser
{
	MOCK_METHOD1(parse_character_impl, void(char));
	MOCK_METHOD0(parse_number_impl, void());
	MOCK_METHOD1(parse_bool_value_impl, void(bool));
};
class ParserBase_ValueTest : public Test
{
public:
	Parser_ValueMock p;
};

TEST_F(ParserBase_ValueTest, parser_provides_interface_to_parse_numbers)
{
	EXPECT_CALL(p, parse_number_impl());
	p.parse("456");
}
TEST_F(ParserBase_ValueTest, parser_skips_spaces_and_tabs)
{
	EXPECT_CALL(p, parse_number_impl())
		.Times(2);

	p.parse("\t456");
	p.parse("  456");
}
TEST_F(ParserBase_ValueTest, parser_skips_new_lines)
{
	EXPECT_CALL(p, parse_number_impl())
		.Times(2);

	p.parse("\n\n456");
	p.parse("\t \n \t  456");
}
TEST_F(ParserBase_ValueTest, parser_provides_interface_to_parse_unary_operators)
{
	EXPECT_CALL(p, parse_number_impl())
		.Times(2);

	p.parse("-456");
	p.parse("+736");
}
TEST_F(ParserBase_ValueTest, parser_provides_interface_to_reset_the_state_of_the_parser)
{
	EXPECT_CALL(p, parse_number_impl())
		.Times(2);

	p.parse("-456");
	p.parse("+869");
}
TEST_F(ParserBase_ValueTest, parser_handles_the_case_where_the_unary_operator_is_separated_by_an_space)
{
	EXPECT_CALL(p, parse_number_impl());
	p.parse("-  248");
}
TEST_F(ParserBase_ValueTest, parser_identifies_boolean_values)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_bool_value_impl(true));
	EXPECT_CALL(p, parse_bool_value_impl(false));

	p.parse("true");
	p.parse("false");
}
TEST_F(ParserBase_ValueTest, parser_identifies_characters)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_character_impl('?'));
	EXPECT_CALL(p, parse_character_impl('!'));

	p.parse("'?'");
	p.parse("'!'");
}

#pragma endregion

#pragma region // Equation parsing test

struct Parser_EquationMock : public DummyParser
{
	MOCK_METHOD0(parse_number_impl, void());
	MOCK_METHOD1(parse_operator_impl, void(OperatorType));
	MOCK_METHOD1(tie_equation_impl, void(std::size_t));
};
class ParserBase_EquationTest : public Test
{
protected:
	Parser_EquationMock p;
};
TEST_F(ParserBase_EquationTest, parser_parses_simple_equations)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_number_impl());
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));
	EXPECT_CALL(p, parse_number_impl());
	EXPECT_CALL(p, tie_equation_impl(1));

	p.parse("456 + 248");

	EXPECT_CALL(p, parse_number_impl());
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::SUB));
	EXPECT_CALL(p, parse_number_impl());
	EXPECT_CALL(p, tie_equation_impl(1));

	p.parse("-248 - 264");
}
TEST_F(ParserBase_EquationTest, parser_parses_larger_simple_equations)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_number_impl());	// 238
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));	// +
	EXPECT_CALL(p, parse_number_impl());	// 937
	EXPECT_CALL(p, parse_operator_impl(OperatorType::SUB));	// -
	EXPECT_CALL(p, parse_number_impl());	// 16
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));	// +
	EXPECT_CALL(p, parse_number_impl());	// -2
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, tie_equation_impl(3));

	p.parse("238 + 937 - 16 + -2");
}

TEST_F(ParserBase_EquationTest, parser_parses_complex_equations_1)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_number_impl());	// 238
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));	// +
	EXPECT_CALL(p, parse_number_impl());	// 937
	EXPECT_CALL(p, parse_operator_impl(OperatorType::SUB));	// -
	EXPECT_CALL(p, parse_number_impl());	// 16
	EXPECT_CALL(p, tie_equation_impl(1));	// (937 - 16)

	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));	// +
	EXPECT_CALL(p, parse_number_impl());	// -2
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, tie_equation_impl(2));	// 238 + (937 - 16) + -2

	p.parse("238 + (937 - 16) + -2");
}
TEST_F(ParserBase_EquationTest, parser_parses_complex_equations_2)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_number_impl());	// 238
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));	// +
	EXPECT_CALL(p, parse_number_impl());	// 937
	EXPECT_CALL(p, parse_operator_impl(OperatorType::SUB));	// -
	EXPECT_CALL(p, parse_number_impl());	// 16
	EXPECT_CALL(p, tie_equation_impl(1));	// (937 - 16)

	EXPECT_CALL(p, parse_operator_impl(OperatorType::MUL));	// *
	EXPECT_CALL(p, parse_number_impl());	// -2
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, tie_equation_impl(1));	// ((937 - 16) + -2)
	EXPECT_CALL(p, tie_equation_impl(1));	// 238 + ((937 - 16) + -2)

	p.parse("238 + ((937 - 16) * -2)");
}
TEST_F(ParserBase_EquationTest, parser_parses_complex_equations_3)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_number_impl());	// 238
	EXPECT_CALL(p, parse_operator_impl(_));	// +
	EXPECT_CALL(p, parse_number_impl());	// 937
	EXPECT_CALL(p, parse_operator_impl(_));	// -
	EXPECT_CALL(p, parse_number_impl());	// 16
	EXPECT_CALL(p, tie_equation_impl(1));	// (937 - 16)

	EXPECT_CALL(p, parse_operator_impl(_));	// +
	EXPECT_CALL(p, parse_number_impl());	// -2
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, tie_equation_impl(1));	// ((937 - 16) + -2)

	EXPECT_CALL(p, parse_operator_impl(_));	// -
	EXPECT_CALL(p, parse_number_impl());	// 10
	EXPECT_CALL(p, tie_equation_impl(2));	// 238 + ((937 - 16) + -2) - 10

	p.parse("((238 + ((937 - 16) + (-2)) - 10))");
}

#pragma endregion

#pragma region // Error reporting tests

class ParserBaseErrorReporting : public Test
{
protected:
	DummyParser p;
};

TEST_F(ParserBaseErrorReporting, parser_throws_when_parenthesis_are_not_correct_in_an_equation)
{
	try
	{
		p.parse("238 + (937 - 16");
		FAIL();
	}
	catch (const except::ParseException & ex)
	{
		ASSERT_EQ(ex.get_line(), std::size_t{ 1 });
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}
TEST_F(ParserBaseErrorReporting, parser_identifies_bad_operator_equals)
{
	try
	{
		p.parse("var b = * 3");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}
TEST_F(ParserBaseErrorReporting, parser_identifies_correctly_the_line_where_the_error_happened)
{
	try
	{
		p.parse("var a = 0\n\t\nvar b = * 3");
		FAIL();
	}
	catch (const except::ParseException & ex)
	{
		ASSERT_EQ(ex.get_line(), std::size_t{ 3 });
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}
TEST_F(ParserBaseErrorReporting, parser_identifies_invalid_operators_in_equations)
{
	try
	{
		p.parse("3 + 2 + * 65");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}

#pragma endregion

#pragma region // Variable parsing test

struct Parser_VariableMock : public DummyParser
{
	MOCK_METHOD3(parse_variable_impl, void(const char *, std::size_t, bool));

	MOCK_METHOD1(tie_equation_impl, void(std::size_t));
	MOCK_METHOD0(tie_assignment_operator_impl, void());
};

class ParserBase_VariableTest : public Test
{
protected:
	Parser_VariableMock p;
};

TEST_F(ParserBase_VariableTest, parser_can_identify_var_keyword)
{
	EXPECT_CALL(p, parse_variable_impl(_, 1, true));

	p.parse("var a");
}
TEST_F(ParserBase_VariableTest, parser_can_identify_equal_operator)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_variable_impl(_, 3, true));		// var a
	EXPECT_CALL(p, tie_assignment_operator_impl());	// =

	p.parse("var aba = 0");
}
TEST_F(ParserBase_VariableTest, variable_names_can_have_underscores)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_variable_impl(_, 4, true));	// var ab_c
	EXPECT_CALL(p, tie_assignment_operator_impl());		// =

	p.parse("var ab_c = 0");
}
TEST_F(ParserBase_VariableTest, variable_names_can_have_numbers_at_a_location_that_is_not_the_first)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_variable_impl(_, _, _))
		.Times(2);

	p.parse("var ab_c3");
	p.parse("var ab32foo");
}
TEST_F(ParserBase_VariableTest, parser_parses_an_equation_followed_by_a_variable_declaration)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_variable_impl(_, _, true));// var a
	EXPECT_CALL(p, tie_equation_impl(2));			// 2 + 4 * 3
	EXPECT_CALL(p, tie_assignment_operator_impl());	// =

	p.parse("var a = 2 + 4 * 3");

	EXPECT_CALL(p, parse_variable_impl(_, _, true));// var a
	EXPECT_CALL(p, tie_equation_impl(1));			// 2 + 4
	EXPECT_CALL(p, tie_equation_impl(1));			// 3 + 3
	EXPECT_CALL(p, tie_equation_impl(2));			// (2 + 4) * 3 / (3 + 3)
	EXPECT_CALL(p, tie_assignment_operator_impl());	// =

	p.parse("var a = (2 + 4) * 3 / (3 + 3)");
}
TEST_F(ParserBase_VariableTest, parser_can_parse_multiple_variables)
{
	EXPECT_CALL(p, parse_variable_impl(_, 1, true))
		.Times(3);	// var a, var b, var c
	EXPECT_CALL(p, parse_variable_impl(_, 1, false))
		.Times(2);	// a + b
	EXPECT_CALL(p, tie_assignment_operator_impl())
		.Times(3);
	EXPECT_CALL(p, tie_equation_impl(2));	// a + b * 2

	p.parse(R"script(
				var a = 5
				var b = 3
				var c = a + b * 2
			)script");
}
TEST_F(ParserBase_VariableTest, parser_can_parse_reasigned_variables)
{
	EXPECT_CALL(p, parse_variable_impl(_, 2, true))
		.Times(3);	// var a1, var a2, var a3
	EXPECT_CALL(p, parse_variable_impl(_, 2, false))
		.Times(5);	// a1 + a2
	EXPECT_CALL(p, tie_assignment_operator_impl())
		.Times(4);
	EXPECT_CALL(p, tie_equation_impl(_))
		.Times(2);	// a1 + a2 * 2, a3 * a3

	p.parse(R"script(
				var a1 = 5
				var a2 = 3
				var a3 = a1 + a2 * 2
				a3 = a3 * a3
			)script");
}

struct Parser_StringMock : public DummyParser
{
	MOCK_METHOD2(parse_string_impl, void(const char *, std::size_t));
};

class ParserBase_StringTest : public Test
{
protected:
	Parser_StringMock p;
};
TEST_F(ParserBase_StringTest, parser_can_parse_strings)
{
	EXPECT_CALL(p, parse_string_impl(_, 13));

	p.parse(R"script(
				var a1 = "Hellow World!"
			)script");
}
TEST_F(ParserBase_StringTest, string_can_have_new_lines_and_tabs_in_it)
{
	EXPECT_CALL(p, parse_string_impl(_, 5));

	p.parse("var a1 = \"A\tB\nC\"");
}
TEST_F(ParserBase_StringTest, parser_can_identify_a_not_correclty_ended_string)
{
	try
	{
		p.parse("\"this should have a closing quote\n\tvar a = 3");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}

#pragma endregion

#pragma region // Unary operator test

struct Parser_UnaryOperatorMock : public DummyParser
{
	MOCK_METHOD1(parse_operator_impl, void(OperatorType));
	MOCK_METHOD3(parse_variable_impl, void(const char *, std::size_t, bool));
};
class ParserBase_UnaryOperatorTest : public Test
{
protected:
	Parser_UnaryOperatorMock p;
};

TEST_F(ParserBase_UnaryOperatorTest, parser_can_identify_unary_plus_and_minus_in_variables)
{
	EXPECT_CALL(p, parse_variable_impl(_, _, _)).Times(4);

	InSequence dummy;
	EXPECT_CALL(p, parse_operator_impl(OperatorType::MUL));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_MINUS));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));
	p.parse("a * -b + 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::MUL));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::UNARY_PLUS));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD));
	p.parse("a * +b + 5");
}
TEST_F(ParserBase_UnaryOperatorTest, parser_can_identify_pre_increment_and_decrement)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::PRE_INC));
	p.parse("++a");

	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::PRE_DEC));
	p.parse("--a");
}
TEST_F(ParserBase_UnaryOperatorTest, parser_can_identify_post_increment_and_decrement)
{
	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::POST_INC));
	p.parse("a++");

	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::POST_DEC));
	p.parse("a--");
}
TEST_F(ParserBase_UnaryOperatorTest, parser_can_identify_logical_not)
{
	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LOGIC_NOT));
	p.parse("!a");
}
TEST_F(ParserBase_UnaryOperatorTest, parser_can_identify_bitwise_not)
{
	EXPECT_CALL(p, parse_variable_impl(_, _, _));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::BITWISE_NOT));
	p.parse("~a");
}

#pragma endregion

#pragma region // More operator tests

struct Parser_MoreOperatorsMock : public DummyParser
{
	MOCK_METHOD1(parse_operator_impl, void(OperatorType));
};
class ParserBase_MoreOperatorsTest : public Test
{
protected:
	Parser_MoreOperatorsMock  p;
};

TEST_F(ParserBase_MoreOperatorsTest, parser_can_parse_bitwhise_operators)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::AND));
	p.parse(R"script(
				var a = 5
				a & 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::XOR));
	p.parse(R"script(
				var a = 5
				a ^ 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::OR));
	p.parse(R"script(
				var a = 5
				a | 3
			)script");
}
TEST_F(ParserBase_MoreOperatorsTest, parser_can_parse_bitshifting_operators)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LEFT_SHIFT));
	p.parse("100 << 3");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::RIGHT_SHIFT));
	p.parse("100 >> 3");
}

class ParserBase_CompoundAssignmentTest : public Test
{
protected:
	Parser_MoreOperatorsMock p;
};

TEST_F(ParserBase_CompoundAssignmentTest, parser_can_parse_compound_assignments)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::ADD_EQ));
	p.parse(R"script(
				var a = 5
				a += 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::SUB_EQ));
	p.parse(R"script(
				var a = 5
				a -= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::MUL_EQ));
	p.parse(R"script(
				var a = 5
				a *= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::DIV_EQ));
	p.parse(R"script(
				var a = 5
				a /= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::MOD_EQ));
	p.parse(R"script(
				var a = 5
				a %= 3
			)script");
}
TEST_F(ParserBase_CompoundAssignmentTest, parser_can_parse_bitwise_compound_assignments)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::AND_EQ));
	p.parse(R"script(
				var a = 5
				a &= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::XOR_EQ));
	p.parse(R"script(
				var a = 5
				a ^= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::OR_EQ));
	p.parse(R"script(
				var a = 5
				a |= 3
			)script");
}
TEST_F(ParserBase_CompoundAssignmentTest, parser_can_parse_bitsifting_compound_assignments)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::RIGHT_SHIFT_EQ));
	p.parse(R"script(
				var a = 5
				a >>= 3
			)script");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::LEFT_SHIFT_EQ));
	p.parse(R"script(
				var a = 5
				a <<= 3
			)script");
}
#pragma endregion

#pragma region // Relational operators

struct Parser_RelationalOperatorsMock : public DummyParser
{
	MOCK_METHOD1(parse_operator_impl, void(OperatorType));
};
class Parser_RelationalOperatorsTest : public Test
{
protected:
	Parser_RelationalOperatorsMock p;
};

TEST_F(Parser_RelationalOperatorsTest, parser_identifies_relational_operators)
{
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LESS));
	p.parse("3 < 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::LESS_EQ));
	p.parse("3 <= 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::GREATER));
	p.parse("3 > 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::GREATER_EQ));
	p.parse("3 >= 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::EQEQ));
	p.parse("3 == 5");

	EXPECT_CALL(p, parse_operator_impl(OperatorType::NOT_EQ));
	p.parse("3 != 5");
}

#pragma endregion

#pragma region // Scope block test

struct Parser_ScopeMock : public DummyParser
{
	MOCK_METHOD1(tie_scope_impl, void(std::size_t));
};

class Parser_ScopeTest : public Test
{
protected:
	Parser_ScopeMock p;
};

TEST_F(Parser_ScopeTest, parser_can_identify_scopes)
{
	EXPECT_CALL(p, tie_scope_impl(0));
	p.parse("{  }");
}
TEST_F(Parser_ScopeTest, parser_provides_correct_number_of_statements_in_the_scope)
{
	EXPECT_CALL(p, tie_scope_impl(3));
	p.parse(R"script(
				{
					var a = 3
					var b = 8 * a
					var c = b / a
				}
			)script");
}

TEST_F(Parser_ScopeTest, parser_throws_when_an_scope_is_not_correctly_closed)
{
	try
	{
		p.parse(R"script(
					{
						var a = 3
						var b = a + 4.3

			)script");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
}

TEST_F(Parser_ScopeTest, parser_throws_when_an_scope_is_not_correctly_opened)
{
	try
	{
		p.parse(R"script(
					var a = 3
					var b = a + 4.3
					
						var c = b - 1
					}
			)script");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
}

#pragma endregion

#pragma region // If statements test

struct Parser_IfStatementMock : public DummyParser
{
	MOCK_METHOD1(tie_if_impl, void(bool));
	MOCK_METHOD1(parse_bool_value_impl, void(bool));
	MOCK_METHOD1(tie_scope_impl, void(std::size_t));
};

class Parser_IfStatementTest : public Test
{
protected:
	Parser_IfStatementMock p;
};

TEST_F(Parser_IfStatementTest, parser_can_identify_if_statements_with_scopes_after_it)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_bool_value_impl(true));
	EXPECT_CALL(p, tie_scope_impl(0));
	EXPECT_CALL(p, tie_if_impl(false));

	p.parse("if (true) { }");
}
TEST_F(Parser_IfStatementTest, parser_can_identify_if_statements_whith_no_scope_after_it)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_bool_value_impl(false));
	EXPECT_CALL(p, tie_scope_impl(1));
	EXPECT_CALL(p, tie_if_impl(false));
	p.parse("if (false)\n\tvar a = 3\n");
}
TEST_F(Parser_IfStatementTest, parser_throws_when_parenthesis_of_an_if_statement_are_not_correct)
{
	EXPECT_CALL(p, parse_bool_value_impl(_));
	try
	{
		p.parse("if (false \n\tvar a = 3\n");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}

	try
	{
		p.parse("if false) \n\tvar a = 3\n");
		FAIL();
	}
	catch (const except::ParseException &)
	{
		SUCCEED();
	}
}
TEST_F(Parser_IfStatementTest, parser_can_identify_else_statements_followed_by_an_if)
{
	EXPECT_CALL(p, parse_bool_value_impl(_));
	EXPECT_CALL(p, tie_scope_impl(_)).Times(2);
	EXPECT_CALL(p, tie_if_impl(true));
	p.parse("if (false) { } else { }");
}
TEST_F(Parser_IfStatementTest, parser_can_identify_else_if_statements_followed_by_an_if)
{
	EXPECT_CALL(p, parse_bool_value_impl(_)).Times(2);
	EXPECT_CALL(p, tie_if_impl(true)).Times(2);
	EXPECT_CALL(p, tie_scope_impl(_)).Times(3);
	p.parse("if (false) { } \n\t else if (true) { } \n\t else { }");
}
TEST_F(Parser_IfStatementTest, parser_can_identify_else_if_statements_followed_by_an_if_2)
{
	EXPECT_CALL(p, tie_if_impl(true)).Times(2);
	EXPECT_CALL(p, tie_scope_impl(1)).Times(3);

	p.parse(R"script(
						var a = 0
						if (a < 0)		{ a = 10 }
						else if (a > 0) { a = 10 }
						else			{ a = 5  }
					)script");
}
TEST_F(Parser_IfStatementTest, singe_line_if_statements_have_an_scope)
{
	EXPECT_CALL(p, tie_if_impl(true)).Times(2);
	EXPECT_CALL(p, tie_scope_impl(1)).Times(3);

	p.parse(R"script(
						var a = 0
						if (a < 0)		a = 10
						else if (a > 0) a = 10
						else			a = 5
					)script");
}

#pragma endregion

#pragma region // While statement test

struct Parser_WhileStatementMock : public DummyParser
{
	MOCK_METHOD0(tie_while_impl, void());
	MOCK_METHOD1(tie_scope_impl, void(std::size_t));
};

class Parser_WhileStatementTest : public Test
{
protected:
	Parser_WhileStatementMock p;
};

TEST_F(Parser_WhileStatementTest, parser_is_able_to_idendtify_while_statements)
{
	InSequence dummy;
	EXPECT_CALL(p, tie_scope_impl(0));
	EXPECT_CALL(p, tie_while_impl());
	p.parse("while(true) { }");
}
TEST_F(Parser_WhileStatementTest, while_loop_with_no_braces_is_parsed_as_a_single_line_scope)
{
	EXPECT_CALL(p, tie_while_impl());
	EXPECT_CALL(p, tie_scope_impl(1));
	p.parse("while(true) var a = 3");
}

#pragma endregion

#pragma region // For statement test

struct Parser_ForStatementMock : public DummyParser
{
	MOCK_METHOD1(tie_scope_impl, void(std::size_t));
	MOCK_METHOD3(tie_for_impl, void(bool, bool, bool));
};

class Parser_ForStatementTest : public Test
{
protected:
	Parser_ForStatementMock p;
};

TEST_F(Parser_ForStatementTest, parser_is_able_to_idendtify_for_statements)
{
	EXPECT_CALL(p, tie_scope_impl(0));
	EXPECT_CALL(p, tie_for_impl(true, true, true));
	p.parse("for (var a = 0; a < 10; ++a) { }");
}
TEST_F(Parser_ForStatementTest, parser_allows_empty_for_loops)
{
	EXPECT_CALL(p, tie_scope_impl(0));
	EXPECT_CALL(p, tie_for_impl(false, false, false));
	p.parse("for ( ; ; ) { }");
}
TEST_F(Parser_ForStatementTest, parser_allows_for_loops_with_some_of_the_statements)
{
	EXPECT_CALL(p, tie_scope_impl(0)).Times(4);

	EXPECT_CALL(p, tie_for_impl(true, false, false));
	p.parse("for (var a = 0; ; ) { }");

	EXPECT_CALL(p, tie_for_impl(false, true, false));
	p.parse("for ( ; false; ) { }");

	EXPECT_CALL(p, tie_for_impl(false, false, true));
	p.parse("for ( ; ; true) { }");

	EXPECT_CALL(p, tie_for_impl(true, false, true));
	p.parse("for ( var a = 2 ; ; a++ ) { }");
}
TEST_F(Parser_ForStatementTest, for_loop_asumes_single_line_scope_if_there_are_no_braces)
{
	EXPECT_CALL(p, tie_scope_impl(1));
	EXPECT_CALL(p, tie_for_impl(_, _, _));
	p.parse("for ( var a = 2 ; ; a = 2 ) a += 1");
}
TEST_F(Parser_ForStatementTest, nested_for_loops_are_parsed_correctly)
{
	EXPECT_CALL(p, tie_scope_impl(1))
		.Times(2);
	EXPECT_CALL(p, tie_for_impl(_, _, _))
		.Times(2);

	p.parse(R"script(
	
	var count = 0
	var c = 10
	for (var a = 0; a < c; ++a)
	{
		for (var b = 0; b < c; ++b)
			count += 1
	}

)script"); 
}

#pragma endregion

class Parser_BooleanMock : public DummyParser
{
public:
	MOCK_METHOD1(parse_bool_value_impl, void(bool));
	MOCK_METHOD1(parse_operator_impl, void(OperatorType));
	MOCK_METHOD1(tie_equation_impl, void(std::size_t));
};
class Parser_BooleanTest : public Test
{
protected:
	Parser_BooleanMock p;
};
TEST_F(Parser_BooleanTest, can_assign_combinations_of_logical_operations_that_end_up_evaluating_to_a_boolean_1)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_bool_value_impl(true));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LOGIC_OR));
	EXPECT_CALL(p, parse_bool_value_impl(false));
	EXPECT_CALL(p, tie_equation_impl(1));

	p.parse(R"script(
						var a = (true || false)
			)script");
}
TEST_F(Parser_BooleanTest, can_assign_combinations_of_logical_operations_that_end_up_evaluating_to_a_boolean_2)
{
	InSequence dummy;
	EXPECT_CALL(p, parse_bool_value_impl(true));
	EXPECT_CALL(p, parse_bool_value_impl(false));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LOGIC_OR));
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LOGIC_AND));
	EXPECT_CALL(p, parse_bool_value_impl(true));
	EXPECT_CALL(p, tie_equation_impl(1)).Times(2);
	EXPECT_CALL(p, parse_operator_impl(OperatorType::LOGIC_AND));
	EXPECT_CALL(p, tie_equation_impl(1));

	p.parse(R"script(
						var a = true
						var b = false
						if (a || (b && true)) c = 1
						if (a && b) d = 1
			)script");
}

class Parser_VectorMock : public DummyParser
{
public:
	MOCK_METHOD1(tie_vector_decl_impl, void(std::size_t));
	MOCK_METHOD0(tie_vector_access_impl, void());
};
class Parser_VectorTest : public Test
{
protected:
	Parser_VectorMock p;
};
TEST_F(Parser_VectorTest, parser_can_identify_vector_notation)
{
	EXPECT_CALL(p, tie_vector_decl_impl(0));
	p.parse("var v = []");
}
TEST_F(Parser_VectorTest, parser_identifies_correctly_the_number_of_parameters)
{
	EXPECT_CALL(p, tie_vector_decl_impl(3));
	p.parse("var v = [1, false, 3 + 5]");

	EXPECT_CALL(p, tie_vector_decl_impl(5));
	p.parse("var v = [\"Hellow!!\", true, 2.3, -4, false]");
}
TEST_F(Parser_VectorTest, parser_identifies_correctly_vector_of_vectors)
{
	EXPECT_CALL(p, tie_vector_decl_impl(1));
	EXPECT_CALL(p, tie_vector_decl_impl(2));
	EXPECT_CALL(p, tie_vector_decl_impl(3));
	p.parse("var v = [1, [false, [\"a\"] ], 3 + 5]");
}
TEST_F(Parser_VectorTest, parser_identifies_vector_access)
{
	EXPECT_CALL(p, tie_vector_access_impl());
	p.parse("v[0]");

	EXPECT_CALL(p, tie_vector_access_impl());
	p.parse("var a = v[this_is_a_variable + 1]");
}
TEST_F(Parser_VectorTest, parser_identifies_vector_of_vectors_access)
{
	EXPECT_CALL(p, tie_vector_access_impl()).Times(2);
	p.parse("v[0][a]");
}
TEST_F(Parser_VectorTest, parser_identifies_vector_access_after_function_call)
{
	EXPECT_CALL(p, tie_vector_access_impl()).Times(2);
	p.parse("get_point_vector()[0][a]");
}

class Parser_FunctionCallMock : public DummyParser
{
public:
	MOCK_METHOD3(tie_global_function_call_impl, void(const char *, std::size_t, std::size_t));
};
class Parser_FunctionCallTest : public Test
{
protected:
	Parser_FunctionCallMock p;
};
TEST_F(Parser_FunctionCallTest, parser_can_identify_function_calls)
{
	EXPECT_CALL(p, tie_global_function_call_impl(_, 3, 0));
	p.parse("foo()");
}
TEST_F(Parser_FunctionCallTest, parser_parses_correctly_all_function_parameters)
{
	EXPECT_CALL(p, tie_global_function_call_impl(_, 15, 4));
	p.parse("pretty_function(a, a + b, -3, \"Hi!!\")");
}
TEST_F(Parser_FunctionCallTest, parser_parses_correctly_function_calls_within_a_function_call)
{
	EXPECT_CALL(p, tie_global_function_call_impl(_, 3, 1));
	EXPECT_CALL(p, tie_global_function_call_impl(_, 6, 3));
	p.parse("thefoo(a, bar(2), false)");
}
TEST_F(Parser_FunctionCallTest, parser_parses_correctly_function_calls_within_equations)
{
	EXPECT_CALL(p, tie_global_function_call_impl(_, 3, 2));
	p.parse("2 + b * -pow(c, 3) * z");
}

class Parser_ClassMemberMock : public DummyParser
{
public:
	MOCK_METHOD3(tie_member_function_call_impl, void(const char *, std::size_t, std::size_t));
	MOCK_METHOD2(parse_member_variable_impl, void(const char *, std::size_t));
};
class Parser_ClassMemberTest : public Test
{
protected:
	Parser_ClassMemberMock p;
};
TEST_F(Parser_ClassMemberTest, parser_parses_correctly_calls_to_member_functions)
{
	EXPECT_CALL(p, tie_member_function_call_impl(_, 6, 0));
	p.parse("a.update()");

	EXPECT_CALL(p, tie_member_function_call_impl(_, 7, 2));
	p.parse("a.set_pos(2, 6)");
}
TEST_F(Parser_ClassMemberTest, parser_parses_correctly_calls_to_recursive_member_functions)
{
	EXPECT_CALL(p, tie_member_function_call_impl(_, 6, 0));
	p.parse("a.update()");

	EXPECT_CALL(p, tie_member_function_call_impl(_, 7, 0));
	EXPECT_CALL(p, tie_member_function_call_impl(_, 5, 1));
	p.parse("a.get_pos().set_x(3)");

	EXPECT_CALL(p, tie_member_function_call_impl(_, 7, 2));
	p.parse("a.set_pos(2, 6)");
}
TEST_F(Parser_ClassMemberTest, parser_parses_correctly_accesses_to_member_variables)
{
	EXPECT_CALL(p, parse_member_variable_impl(_, 7));
	p.parse("a.mem_var");
}
TEST_F(Parser_ClassMemberTest, parser_parses_correctly_member_acces_from_return_value_of_function_calls)
{
	EXPECT_CALL(p, tie_member_function_call_impl(_, 3, 3));
	p.parse("foo().bar(1, 2, [ 1, 2, 3 ])");

	EXPECT_CALL(p, parse_member_variable_impl(_, 3));
	p.parse("foo().bar");
}

TEST_F(Parser_ClassMemberTest, parser_parses_correctly_accesses_to_recursive_member_variables_and_functions)
{
	EXPECT_CALL(p, parse_member_variable_impl(_, 7));
	EXPECT_CALL(p, parse_member_variable_impl(_, 1));
	EXPECT_CALL(p, parse_member_variable_impl(_, 3));
	p.parse("a.mem_var.b.foo");

	EXPECT_CALL(p, parse_member_variable_impl(_, 7));
	EXPECT_CALL(p, tie_member_function_call_impl(_, _, _));
	EXPECT_CALL(p, parse_member_variable_impl(_, 3));
	p.parse("a.mem_var.foo().bar");

	EXPECT_CALL(p, parse_member_variable_impl(_, 7));
	EXPECT_CALL(p, tie_member_function_call_impl(_, _, _));
	EXPECT_CALL(p, parse_member_variable_impl(_, 3));
	p.parse("a.mem_var[4].foo()[4].bar[4]");
}