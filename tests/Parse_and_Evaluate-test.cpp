
#include "gmock\gmock.h"
using namespace testing;

#include "Parse\Parser.h"			// parser::Parser
#include "Runtime\DispatchEngine.h"	// runtime::DispatchEngine
#include "Runtime\RuntimeException.h"

class ParserEvaluationTest : public Test
{
public:
	parse::Parser p;
	runtime::DispatchEngine eng;

	BoxedValue evaluate_parsed_data()
	{
		return eng.evaluate(*p.get_root());
	}

	template <typename T>
	T evaluate_parsed_data()
	{
		const BoxedValue result = evaluate_parsed_data();
		return boxed_cast<T>(result);
	}

	BoxedValue parse_and_evaluate(const char * str)
	{
		p.parse(str);
		return evaluate_parsed_data();
	}

	template <typename T>
	T parse_and_evaluate(const char * str)
	{
		return boxed_cast<T>(parse_and_evaluate(str));
	}
};

class EquationParseEvalTest : public ParserEvaluationTest {};
TEST_F(EquationParseEvalTest, equation_parser_is_able_to_parse_and_evaluate_simple_equations)
{
	p.parse("2 + 3");
	ASSERT_EQ(evaluate_parsed_data<int>(), 5);

	p.parse("2 - 3");
	ASSERT_EQ(evaluate_parsed_data<int>(), -1);

	p.parse("2 * -3");
	ASSERT_EQ(evaluate_parsed_data<int>(), -6);

	p.parse("15 / 3");
	ASSERT_EQ(evaluate_parsed_data<int>(), 5);
}
TEST_F(EquationParseEvalTest, equation_parser_is_able_to_parse_and_evaluate_equations_with_precedence_operators_1)
{
	p.parse("2 + 3 * (1 + 3)");
	ASSERT_EQ(evaluate_parsed_data<int>(), 2 + 3 * (1 + 3));

	p.parse("3 * (1 + 3) - 4 / 2");
	ASSERT_EQ(evaluate_parsed_data<int>(), 3 * (1 + 3) - 4 / 2);
}
TEST_F(EquationParseEvalTest, equation_parser_is_able_to_parse_and_evaluate_equations_with_precedence_operators_2)
{
	p.parse("2 + 3 * (1 + 3) / 4 + 1");
	ASSERT_EQ(evaluate_parsed_data<int>(), 2 + 3 * (1 + 3) / 4 + 1);

	p.parse("2 + 3 * (1 + 3) / 4 + 1.85");
	ASSERT_EQ(evaluate_parsed_data<float>(), 2 + 3 * (1 + 3) / 4 + 1.85f);
}
TEST_F(EquationParseEvalTest, equation_parser_identifies_modulus_operator)
{
	p.parse("50 % 10");
	ASSERT_EQ(evaluate_parsed_data<int>(), 50 % 10);

	// cannot perform modulus of a float
	p.parse("4.2 % 2");
	try
	{
		evaluate_parsed_data();
		FAIL();
	}
	catch (...) {}
}


class VariableParseEvalTest : public ParserEvaluationTest {};
TEST_F(VariableParseEvalTest, variables_are_correctly_parsed)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	p.parse("var a = 0");
	evaluate_parsed_data();

	ASSERT_EQ(eng.get_variable_num(), 1u);

	try
	{
		eng.get_variable("a");
	}
	catch (...)
	{
		FAIL();
	}
}
TEST_F(VariableParseEvalTest, variables_assign_values_correctly)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	parse_and_evaluate("var a = 50");

	ASSERT_EQ(eng.get_variable_num(), 1u);
	ASSERT_EQ(eng.get_variable_as<int>("a"), 50);
}
TEST_F(VariableParseEvalTest, variables_can_change_the_types_they_are_storing_dynamically)
{
	// TODO(Borja): are we sure we want this?

	//parse_and_evaluate("var a = 50 \n a = 3.5");
	//
	//const BoxedValue * a = eng.get_variable("a");
	//ASSERT_EQ(a->get_type_info(), get_type_info<float>());
	//ASSERT_EQ(boxed_cast<float>(*a), 3.5f);
}
TEST_F(VariableParseEvalTest, variables_assign_values_correctly_2)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	parse_and_evaluate(R"script(
						var the_truth = false
						var the_real_truth = true
			)script");

	ASSERT_FALSE(boxed_cast<bool>(*eng.get_variable("the_truth")));
	ASSERT_TRUE(boxed_cast<bool>(*eng.get_variable("the_real_truth")));
}
TEST_F(VariableParseEvalTest, variables_assign_values_correctly_3)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	parse_and_evaluate("var a = 50 \n var b = 8");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 50);
	ASSERT_EQ(eng.get_variable_as<int>("b"), 8);
}
TEST_F(VariableParseEvalTest, variables_can_be_used_in_equations)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	parse_and_evaluate(R"script(
						var a = 5
						var b = 3
						var c = a + b * 2
		)script");

	ASSERT_EQ(eng.get_variable_num(), 3u);
	ASSERT_EQ(eng.get_variable_as<int>("c"), 5 + 3 * 2);
}
TEST_F(VariableParseEvalTest, variables_can_be_reassigned_before_declaration)
{
	parse_and_evaluate(R"script(
						var a0 = 5
						var a1 = 3
						var a2 = a0 + a1 * 2
						a2 = a2 * a2
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a2"), 11 * 11);
}
TEST_F(VariableParseEvalTest, variables_can_hold_strings)
{
	parse_and_evaluate("var a0 = \"test\"");
	ASSERT_EQ(eng.get_variable_as<std::string>("a0"), "test");
}
TEST_F(VariableParseEvalTest, keyworkds_cannot_be_variables)
{
	try
	{
		parse_and_evaluate("var if = 0");
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

	parse_and_evaluate("var true_ = 0 \n var false3 = 2 \n var var_2 = 3");

}


class OperatorParseEvalTest : public ParserEvaluationTest {};
TEST_F(OperatorParseEvalTest, unary_plus_and_minus_operators_work_as_expected_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 5
						var b = -a
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("b"), -5);

	parse_and_evaluate(R"script(
						var a = 5
						var b = +a
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("b"), 5);
}
TEST_F(OperatorParseEvalTest, unary_pre_and_post_increment_mechanics)
{
	parse_and_evaluate(R"script(
						var a = 5
						++a
						a++
						var b = 2 + a--
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 6);
	ASSERT_EQ(eng.get_variable_as<int>("b"), 9);
}
TEST_F(OperatorParseEvalTest, bitwise_and_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a = a & 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 & 3);
}
TEST_F(OperatorParseEvalTest, bitwise_xor_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a = a ^ 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 ^ 3);
}
TEST_F(OperatorParseEvalTest, bitwise_or_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a = a | 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 | 3);
}
TEST_F(OperatorParseEvalTest, logical_operators_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = true || false
						var b = true && false
						var c = 0
						var d = 0
						if (a || (b && true)) c = 1
						if ((b && true) || a) d = 1
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("c"), 1);
	ASSERT_EQ(eng.get_variable_as<int>("d"), 1);
}
TEST_F(OperatorParseEvalTest, bit_shifting_operators_work_as_expected)
{
	ASSERT_EQ(parse_and_evaluate<int>("1245 >> 8"), 1245 >> 8);
	ASSERT_EQ(parse_and_evaluate<int>("11 << 8"), 11 << 8);
}
TEST_F(OperatorParseEvalTest, unary_logical_not)
{
	ASSERT_EQ(parse_and_evaluate<bool>("!true"), false);
	ASSERT_EQ(parse_and_evaluate<bool>("var a = false \n a = !a"), true);
}
TEST_F(OperatorParseEvalTest, unary_bitwise_not)
{
	ASSERT_EQ(parse_and_evaluate<int>("~2174"), ~2174);
}
TEST_F(OperatorParseEvalTest, strings_can_use_add_operator)
{
	ASSERT_EQ(parse_and_evaluate<std::string>("\"He\" + \"llo\""), std::string{ "Hello" });
}


class CompaundOperatorsTest : public ParserEvaluationTest {};
TEST_F(CompaundOperatorsTest, relational_operators_works_as_expected)
{
	ASSERT_EQ(parse_and_evaluate<bool>("1 < 4"), 1 < 4);
	ASSERT_EQ(parse_and_evaluate<bool>("4 <= 4"), 4 <= 4);
	ASSERT_EQ(parse_and_evaluate<bool>("2 > 4"), 2 > 4);
	ASSERT_EQ(parse_and_evaluate<bool>("2 >= 4"), 2 >= 4);
	ASSERT_EQ(parse_and_evaluate<bool>("6 >= 4"), 6 >= 4);
	ASSERT_EQ(parse_and_evaluate<bool>("6 == 4"), 6 == 4);
	ASSERT_EQ(parse_and_evaluate<bool>("6 != 4"), 6 != 4);
}
TEST_F(CompaundOperatorsTest, operator_plus_equal_works_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 5
						a += 3
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 + 3);
}
TEST_F(CompaundOperatorsTest, operator_minus_equal_works_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 5
						a -= 3
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 - 3);
}
TEST_F(CompaundOperatorsTest, operator_mul_equal_works_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 5
						a *= 3
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 * 3);
}
TEST_F(CompaundOperatorsTest, operator_div_equal_works_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 30.3
						a /= 3
			)script");

	ASSERT_EQ(eng.get_variable_as<float>("a"), 30.3f / 3);
}
TEST_F(CompaundOperatorsTest, operator_mod_equal_works_with_variables)
{
	parse_and_evaluate(R"script(
						var a = 5
						a %= 3
			)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 % 3);
}
TEST_F(CompaundOperatorsTest, bitwise_and_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a &= 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 & 3);
}
TEST_F(CompaundOperatorsTest, bitwise_xor_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a ^= 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 ^ 3);
}
TEST_F(CompaundOperatorsTest, bitwise_or_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a |= 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 | 3);
}
TEST_F(CompaundOperatorsTest, bit_shifting_operators_work_as_expected)
{
	parse_and_evaluate(R"script(
						var a = 5
						a >>= 1
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 >> 1);

	parse_and_evaluate(R"script(
						var a = 5
						a <<= 3
			)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 5 << 3);
}
TEST_F(CompaundOperatorsTest, strings_can_use_add_equal_operator)
{
	parse_and_evaluate<std::string>(R"script(
var a = "Hel"
a += "lo "
a += "Worl" + "d!!"
)script");

	ASSERT_EQ(eng.get_variable_as<std::string>("a"), std::string{ "Hello World!!" });
}

class ScopeParseEvalTest : public ParserEvaluationTest {};
TEST_F(ScopeParseEvalTest, scope_executes_correctly_all_statements_in_it)
{
	p.parse(R"script(
				var a = 4
				{
					var b = 8
					a *= b
				}
			)script");

	evaluate_parsed_data();
	ASSERT_EQ(eng.get_variable_as<int>("a"), 4 * 8);
}
TEST_F(ScopeParseEvalTest, scope_implements_correclty_the_stack)
{
	p.parse(R"script(
				var a = 4
				{
					var a = 8.2
					a *= 3.4
				}
			)script");

	evaluate_parsed_data();
	ASSERT_EQ(eng.get_variable_as<int>("a"), 4);
}


class IfStatementParseEvalTest : public ParserEvaluationTest {};
TEST_F(IfStatementParseEvalTest, if_statement_is_taken_correclty)
{
	parse_and_evaluate(R"script(
						var a = 4
						if (a > 3) a = 0
						if (a < 0) a = 10
					)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 0);
}
TEST_F(IfStatementParseEvalTest, if_statement_evaluates_all_numbers_as_true_but_zero)
{
	parse_and_evaluate(R"script(
						var a = 4
						if (a) a = 0
						if (a) a = 10
					)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 0);
}
TEST_F(IfStatementParseEvalTest, else_if_statement_is_correctly_evaluated)
{
	parse_and_evaluate(R"script(
						var a = 0
						if (a < 0)		a = 10
						else if (a > 0) a = 10
						else			a = 5
					)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5);
}
TEST_F(IfStatementParseEvalTest, if_statement_creates_scopes_correctly)
{
	parse_and_evaluate(R"script(
						var a = 0
						if (a < 0)		a = 10
						else if (a > 0) a = 10
						else			var a = 5
					)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 0);
}

class WhileStatementParseEvalTest : public ParserEvaluationTest {};
TEST_F(WhileStatementParseEvalTest, while_statement_is_correclty_parsed_and_executed)
{
	parse_and_evaluate("var a = 0 \n while (false) a = 1");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 0);

	parse_and_evaluate("var b = 0 \n while(b < 5) b += 2");
	ASSERT_EQ(eng.get_variable_as<int>("b"), 6);
}
TEST_F(WhileStatementParseEvalTest, while_statement_and_pre_post_increment_decrement_test)
{
	parse_and_evaluate(R"script(
var i = 5
var b = 0
while (i-- > 0) b++
						)script");

	int i = 5;
	int b = 0;
	while (i-- > 0) b++;

	ASSERT_EQ(eng.get_variable_as<int>("b"), b);
}


class ForStatementParseEvalTest : public ParserEvaluationTest {};
TEST_F(ForStatementParseEvalTest, for_statement_is_correcltly_built)
{
	parse_and_evaluate("for (var a = 0; a < 5; ++a) {  }");
}
TEST_F(ForStatementParseEvalTest, for_statement_excutes_correctly)
{
	parse_and_evaluate("var b = 0 \n for (var a = 0; a < 5; ++a) { b += 3 }");
	ASSERT_EQ(eng.get_variable_as<int>("b"), 5 * 3);
}
TEST_F(ForStatementParseEvalTest, nested_for_statements_excute_correctly)
{
	parse_and_evaluate(R"script(
	
	var count = 0
	var c = 10
	for (var a = 0; a < c; ++a)
	{
		for (var b = 0; b < c; ++b)
			count += 1
	}

)script");
	int c = eng.get_variable_as<int>("c");
	ASSERT_EQ(eng.get_variable_as<int>("count"), c * c);
}


class VectorParseEvalTest : public ParserEvaluationTest {};
TEST_F(VectorParseEvalTest, vector_initialization_is_correctly_parsed)
{
	parse_and_evaluate("var v = []");
	ASSERT_TRUE(eng.get_variable("v")->is_storing_exactly<std::vector<BoxedValue>>());
}
TEST_F(VectorParseEvalTest, vector_initialization_parses_correctly_all_values)
{
	parse_and_evaluate("var v = [\"Hey!\", true, 6, 1.3]");

	const BoxedValue * v = eng.get_variable("v");
	const auto & real_vector = boxed_cast<std::vector<BoxedValue>>(*v);

	ASSERT_EQ(real_vector.size(), 4);
	ASSERT_EQ(boxed_cast<std::string>(real_vector[0]), std::string{ "Hey!" });
	ASSERT_EQ(boxed_cast<bool>(real_vector[1]), true);
	ASSERT_EQ(boxed_cast<int>(real_vector[2]), 6);
	ASSERT_EQ(boxed_cast<float>(real_vector[3]), 1.3f);
}
TEST_F(VectorParseEvalTest, vector_access_returns_variable_in_the_correct_index)
{
	parse_and_evaluate(R"script(
var v = ["Hey!", true, 2, 1.3]
var a = v[0]
var b = v[1]
var c = v[2]
var d = v[c + 1]
						)script");

	ASSERT_EQ(eng.get_variable_as<std::string>("a"), std::string{ "Hey!" });
	ASSERT_EQ(eng.get_variable_as<bool>("b"), true);
	ASSERT_EQ(eng.get_variable_as<int>("c"), 2);
	ASSERT_EQ(eng.get_variable_as<float>("d"), 1.3f);
}
TEST_F(VectorParseEvalTest, vector_of_vectors_initialization_parses_correctly_all_values)
{
	parse_and_evaluate("var v = [ [ [ 6 ] ] ]");

	const BoxedValue & v = *eng.get_variable("v");
	const auto & v1 = boxed_cast<std::vector<BoxedValue>>(v);
	const auto & v2 = boxed_cast<std::vector<BoxedValue>>(v1[0]);
	const auto & v3 = boxed_cast<std::vector<BoxedValue>>(v2[0]);

	ASSERT_EQ(boxed_cast<int>(v3[0]), 6);
}
TEST_F(VectorParseEvalTest, vector_of_vectors_access_is_correctly_resolved)
{
	parse_and_evaluate(R"script(
var v = ["Hey!", [ true, 2 ], [ 1.3 ]]
var a = v[1][0]
var b = v[1][1]
var c = v[2][b - 2]
var d = v[2][v[1][1] - 2]
						)script");

	ASSERT_EQ(eng.get_variable_as<bool>("a"), true);
	ASSERT_EQ(eng.get_variable_as<int>("b"), 2);
	ASSERT_EQ(eng.get_variable_as<float>("c"), 1.3f);
	ASSERT_EQ(eng.get_variable_as<float>("c"), eng.get_variable_as<float>("d"));
}
TEST_F(VectorParseEvalTest, vector_of_vectors_access_with_vector_as_index_does_not_generate_conflicts)
{
	parse_and_evaluate(R"script(
var v = ["Hey!", [ true, 2 ], [ 1.3, "this is returned" ]]
var a = v[2][ v[1][1] - 1 ]
						)script");

	ASSERT_EQ(eng.get_variable_as<std::string>("a"), std::string{ "this is returned" });
}
TEST_F(VectorParseEvalTest, vector_and_unary_operators_have_the_correct_precedence)
{
	parse_and_evaluate(R"script(
var v = [5, 3, 2]
var a = -v[1]
var b = +v[0]
						)script");

	ASSERT_EQ(eng.get_variable_as<int>("a"), -3);
	ASSERT_EQ(eng.get_variable_as<int>("b"), 5);
}


class GlobalFunctionBidingParseEvalTest : public ParserEvaluationTest
{
public:
	static int my_function()
	{
		return 4;
	}
	static float my_pow(float a, int p)
	{
		return std::pow(a, p);
	}

	template <typename T>
	static T times_2(T a)
	{
		return a * 2;
	}
};
TEST_F(GlobalFunctionBidingParseEvalTest, global_functions_are_correctly_called)
{
	eng.add("returns_four", binds::func(my_function));
	ASSERT_EQ(parse_and_evaluate<int>("returns_four()"), 4);
}
TEST_F(GlobalFunctionBidingParseEvalTest, global_function_parameters_are_correctly_evaluated)
{
	eng.add("pow", binds::func(my_pow));
	ASSERT_EQ(parse_and_evaluate<float>("pow(1.5, 4)"), std::pow(1.5f, 4));

	eng.add("foo", binds::func(my_function));
	ASSERT_EQ(parse_and_evaluate<float>("pow(1.5, foo())"), std::pow(1.5f, my_function()));
}
TEST_F(GlobalFunctionBidingParseEvalTest, global_function_call_throws_if_parameter_number_is_not_valid)
{
	eng.add("foo", binds::func(my_function));
	try
	{
		parse_and_evaluate("foo(1)");
		FAIL();
	}
	catch (const except::RuntimeException &)
	{
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}

	eng.add("pow", binds::func(my_pow));
	try
	{
		parse_and_evaluate("pow()");
		FAIL();
	}
	catch (const except::RuntimeException &)
	{
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}
TEST_F(GlobalFunctionBidingParseEvalTest, can_overload_global_functions)
{
	eng.add("foo", binds::func(my_function));
	eng.add("foo", binds::func(my_pow));

	parse_and_evaluate("assert(foo() == 4, \"\")");
	parse_and_evaluate("var a = foo(1.5, 4)");

	ASSERT_EQ(eng.get_variable_value<float>("a"), std::pow(1.5f, 4));
}
TEST_F(GlobalFunctionBidingParseEvalTest, overloaded_global_function_call_is_correctly_resolved)
{
	eng.add("foo", binds::func(times_2<int>));
	eng.add("foo", binds::func(times_2<float>));

	parse_and_evaluate("assert(foo(1) == 2)");
	parse_and_evaluate("assert(foo(2.31) == 4.62)");
}

class TypeConversionTest : public ParserEvaluationTest {};
TEST_F(TypeConversionTest, can_construct_specific_types_using_ctor_like_sintax)
{
	parse_and_evaluate("assert(int(2.43) == 2)");
}

class AssertionTest : public ParserEvaluationTest {};
TEST_F(AssertionTest, assertion_does_nothing_when_the_result_is_true)
{
	parse_and_evaluate("assert(true, \"\")");
}
TEST_F(AssertionTest, assertion_throws_when_an_assertion_is_given)
{
	try
	{
		parse_and_evaluate("assert(false, \"this should match the exception explanation\")");
		FAIL();
	}
	catch (const except::AssertionFailure & ex)
	{
		ASSERT_TRUE(std::strcmp(ex.what(), "this should match the exception explanation") == 0);
		SUCCEED();
	}
	catch (...)
	{
		FAIL();
	}
}


class MemberFunctionBidingParseEvalTest : public ParserEvaluationTest {};
TEST_F(MemberFunctionBidingParseEvalTest, vector_functions_are_bound_1)
{
	parse_and_evaluate("var v = [ \"foo\", 0, 3.2 ]\nvar b = v.size()");
	ASSERT_EQ(boxed_cast<std::size_t>(*eng.get_variable("b")), 3);
}
TEST_F(MemberFunctionBidingParseEvalTest, vector_functions_are_bound_2)
{
	parse_and_evaluate(R"script(

var v = [ 1, 2, 3 ]
v.push_back(5)

var b = v.size()
var c = v.empty()

assert(v[3] == 5, "")

v.pop_back()
assert(v.size() == 3, "")
assert(v[2] == 3, "")

v.push_back(2)

)script");
	ASSERT_EQ(eng.get_variable_value<std::size_t>("b"), 4);
	ASSERT_EQ(eng.get_variable_value<bool>("c"), false);

	auto & v = boxed_cast<std::vector<BoxedValue>>(*eng.get_variable("v"));
	ASSERT_EQ(boxed_cast<int>(v[0]), 1);
	ASSERT_EQ(boxed_cast<int>(v[1]), 2);
	ASSERT_EQ(boxed_cast<int>(v[2]), 3);
	ASSERT_EQ(boxed_cast<int>(v[3]), 2);
}
TEST_F(MemberFunctionBidingParseEvalTest, vector_functions_are_bound_3)
{
	parse_and_evaluate(R"script(

var v = []

v.reserve(20)
var a = v.capacity()

v.resize(15)
var b = v.size()
var c = v.capacity()

)script");
	ASSERT_EQ(boxed_cast<std::size_t>(*eng.get_variable("a")), 20);
	ASSERT_EQ(boxed_cast<std::size_t>(*eng.get_variable("b")), 15);
	ASSERT_EQ(boxed_cast<std::size_t>(*eng.get_variable("c")), 20);
}
TEST_F(MemberFunctionBidingParseEvalTest, string_functions_are_bound_4)
{
	parse_and_evaluate(R"script(
var str = "Hello world!"

assert(str.size() == 12, "size is wrong")
assert(str.length() == 12, "length is wrong")
str.push_back('?')
assert(str.size() == 13, "push back doesn't work")
assert(str == "Hello world!?", "comparison doesn't work")

)script");
}
TEST_F(MemberFunctionBidingParseEvalTest, string_functions_are_bound_5)
{
	parse_and_evaluate(R"script(
var str = "Hello world!"

assert(str.size() == 12, "size is wrong")
assert(str.length() == 12, "length is wrong")
str.push_back('?')
assert(str.size() == 13, "push back doesn't work")
assert(str == "Hello world!?", "comparison doesn't work")

)script");
}

class CustomTypesTest : public ParserEvaluationTest
{
public:
	struct Bar
	{
		int times_2(int a) { return a * 2; }
	};
	struct Foo
	{
		Foo() = default;
		Foo(float ii) : i{ ii } {}

		void set_i(int ii) { i = (float)ii; }
		void set_i(int ii, int bb) { i = (float)(ii * bb); }
		void set_i(int ii, float bb) { i = ii * bb; }

		float i{ 3 };
		Bar b;
	};
};
TEST_F(CustomTypesTest, user_can_create_custom_types_in_scripts)
{
	eng.add("Foo", binds::ctor<Foo()>());
	eng.add("set_i", binds::func<Foo, void, int>(&Foo::set_i));

	parse_and_evaluate(R"script(
var f1 = Foo()
var f2 = Foo()

f2.set_i(60)
)script");

	ASSERT_EQ(eng.get_variable_as<Foo>("f1").i, 3);
	ASSERT_EQ(eng.get_variable_as<Foo>("f2").i, 60);
}
TEST_F(CustomTypesTest, user_can_overload_constructors)
{
	eng.add("Foo", binds::ctor<Foo()>());
	eng.add("Foo", binds::ctor<Foo(float)>());
	eng.add("set_i", binds::func<Foo, void, int>(&Foo::set_i));

	parse_and_evaluate(R"script(
var f1 = Foo()
var f2 = Foo(5 + 0.2 * 3)
)script");

	ASSERT_EQ(eng.get_variable_as<Foo>("f1").i, 3);
	ASSERT_EQ(eng.get_variable_as<Foo>("f2").i, 5.6f);
}
TEST_F(CustomTypesTest, user_can_overload_member_functions)
{
	eng.add("Foo", binds::ctor<Foo()>());
	eng.add("set_i", binds::func<Foo, void, int>(&Foo::set_i));
	eng.add("set_i", binds::func<Foo, void, int, int>(&Foo::set_i));
	eng.add("set_i", binds::func<Foo, void, int, float>(&Foo::set_i));

	parse_and_evaluate(R"script(

var f1 = Foo()
var f2 = Foo()
var f3 = Foo()

f1.set_i(60)
f2.set_i(20, 2)
f3.set_i(5, 2.5)

)script");

	ASSERT_EQ(eng.get_variable_as<Foo>("f1").i, 60);
	ASSERT_EQ(eng.get_variable_as<Foo>("f2").i, 40);
	ASSERT_EQ(eng.get_variable_as<Foo>("f3").i, 12.5f);
}
TEST_F(CustomTypesTest, can_bind_member_variables_and_access_them_from_script)
{
	eng.add("Foo", binds::ctor<Foo()>());
	eng.add("the_var", binds::var(&Foo::i));

	parse_and_evaluate(R"script(

var f1 = Foo()
f1.the_var = 3 * 8
assert(f1.the_var == 24, "Member variable assignment is not working.")

)script");

	ASSERT_EQ(eng.get_variable_as<Foo>("f1").i, 24);
}
TEST_F(CustomTypesTest, member_variables_of_class_types_work_as_expected)
{
	eng.add("Foo", binds::ctor<Foo()>());
	eng.add("times_2", binds::func(&Bar::times_2));
	eng.add("b", binds::var(&Foo::b));

	parse_and_evaluate(R"script(

assert(Foo().b.times_2(3) == 6)

)script");
}
TEST_F(CustomTypesTest, objects_are_handled_in_different_ways)
{
	eng.add("i", binds::var(&Foo::i));
	eng.add("make_foo_ptr", binds::func<Foo*()>(
		[]() { static Foo f; return &f; }));
	eng.add("make_foo_shared_ptr", binds::func<std::shared_ptr<Foo>()>(
		[]() { return std::make_shared<Foo>(); }));

	parse_and_evaluate(R"script(

var f = make_foo_ptr()
f.i = 6
assert(f.i == 6, "ptr")

var f2 = make_foo_shared_ptr()
f2.i = 20
assert(f2.i == 20, "shared_ptr")

)script");
}

class CallableObjectsTest : public ParserEvaluationTest 
{
public:
	struct Component
	{
		Component()
			: m_multiply{ [](int a, int b) { return a * b; } }
		{}

		std::function<int(int, int)> & get_multiply()
		{
			return m_multiply;
		}

		std::function<int(int, int)> m_multiply;
	};
};
TEST_F(CallableObjectsTest, can_bind_objects_that_overload_call_operator)
{
	std::function<int(int, int)> add = [](int a, int b) { return a + b; };
	eng.add("add", binds::var(add));
	eng.add("()", binds::func(&std::function<int(int, int)>::operator()));

	parse_and_evaluate("var a = add(2, 3)");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 5);
}
TEST_F(CallableObjectsTest, call_operator_overloading_is_correctly_resolved_in_member_variables)
{
	eng.add("Component", binds::ctor<Component()>());
	eng.add("multiply", binds::var(&Component::m_multiply));
	eng.add("()", binds::func(&std::function<int(int, int)>::operator()));

	parse_and_evaluate(R"script(

var cmp = Component()
var six = cmp.multiply(2, 3)
assert(six == 6, "call operator binding in member variable failed")

)script");
}
TEST_F(CallableObjectsTest, DISABLED_call_operator_works_on_returned_variables)
{
	eng.add("Component", binds::ctor<Component()>());
	eng.add("get_multiply", binds::func(&Component::get_multiply));
	eng.add("()", binds::func(&std::function<int(int, int)>::operator()));

	parse_and_evaluate(R"script(

var cmp = Component()
var mul = cmp.get_multiply()
var six = mul(2, 3)
assert(six == 6, "call operator binding in member variable failed")

// this sintax should be supported...
var twelve = cmp.get_multiply()(six, 2)

)script");
}

class GlobalVariableBindingParseEvalTest : public ParserEvaluationTest {};
TEST_F(GlobalVariableBindingParseEvalTest, changing_the_global_variable_from_an_script_changes_the_one_we_access_from_cpp)
{
	int global_int = 0;

	eng.add("the_global_int", binds::var(global_int));
	parse_and_evaluate("the_global_int = 4");

	ASSERT_EQ(global_int, 4);
}
TEST_F(GlobalVariableBindingParseEvalTest, changing_the_global_variable_from_cpp_changes_when_using_it_in_an_script)
{
	int global_int = 0;
	eng.add("the_global_int", binds::var(global_int));

	global_int = 3;
	parse_and_evaluate("var a = the_global_int");

	ASSERT_EQ(eng.get_variable_as<int>("a"), 3);
}


class CommentParseEvaluateTest : public ParserEvaluationTest {};
TEST_F(CommentParseEvaluateTest, single_line_comments_are_ignored)
{
	parse_and_evaluate("// assert(false, \"\") \n var a = 4");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 4);
}
TEST_F(CommentParseEvaluateTest, multiline_comments_are_ignored)
{
	parse_and_evaluate(R"script(
/* 
	assert(false, \"\") 
*/ var a = 4 

/* also ignored 
// 		var b = 2			   */

var b = 3

)script");
	ASSERT_EQ(eng.get_variable_as<int>("a"), 4);
	ASSERT_EQ(eng.get_variable_as<int>("b"), 3);
}


class EvaluationTest : public ParserEvaluationTest {};
TEST_F(EvaluationTest, can_create_and_execute_scripts_within_an_script)
{
	eng.add("parse_and_evaluate", binds::func<void(const char *)>(
		[this](const char * script)
	{
		parse_and_evaluate(script);
	}));

	eng.add("scripting_engine", binds::var(eng));
	eng.add("get_variable", binds::func(&runtime::DispatchEngine::get_variable));

	parse_and_evaluate(R"script(

		parse_and_evaluate("var a = 3 * 4 + 3")
		
		var b = scripting_engine.get_variable("a")
		assert(b == 15)

	)script");
}

TEST_F(EvaluationTest, stress_test)
{
	parse_and_evaluate(R"script(
	
	var count = 0
	var c = 100
	for (var a = 0; a < c; ++a)
	{
		for (var b = 0; b < c; ++b)
			count += 1
	}

)script");

	const int c = eng.get_variable_value<int>("c");
	ASSERT_EQ(eng.get_variable_value<int>("count"), c * c);
}