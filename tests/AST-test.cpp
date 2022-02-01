
#include "gmock\gmock.h"
using namespace testing;

#include "Runtime\AST.h"	// namespace ast
using namespace ast;

#include "Parse\OperatorParsing.h"	// parser::get_operator_type

#include "Runtime\DispatchEngine.h"	// runtime::DispatchEngine

class ASTTest : public Test
{
private:
	runtime::DispatchEngine eng;

public:
	template <typename T>
	T evaluate_node(ASTNode & node)
	{
		return boxed_cast<T>(node.evaluate(eng));
	}
	template <typename T>
	T evaluate_node(const ASTNode & node)
	{
		return boxed_cast<T>(node.evaluate(eng));
	}
};

template <typename T1, typename T2>
std::unique_ptr<BinaryOperator> make_operator(T1 v1, unsigned char op, T2 v2)
{
	return make_operator(v1, parse::get_operator_type(op), v2);
}
std::unique_ptr<BinaryOperator> make_operator(unsigned char op)
{
	return make_operator(parse::get_operator_type(op));
}

class ValueTest : public ASTTest {};

TEST_F(ValueTest, can_be_constructed_out_of_a_number)
{
	const ast::Value num0{ BoxedValue{ 4 } };
	ASSERT_EQ(evaluate_node<int>(num0), 4);

	const ast::Value num1{ BoxedValue{ 16.5f } };
	ASSERT_EQ(evaluate_node<float>(num1), 16.5f);
}
TEST_F(ValueTest, provides_interface_to_create_values_less_verbosely)
{
	auto node = make_value(4);
	ASSERT_EQ(evaluate_node<int>(*node), 4);

	node = make_value(16.5f);
	ASSERT_EQ(evaluate_node<float>(*node), 16.5f);

	node = make_value<std::string>("Hellow World!!");
	ASSERT_EQ(evaluate_node<std::string>(*node), "Hellow World!!");
}

class OperatorTest : public ASTTest {};

TEST_F(OperatorTest, provides_interface_to_create_operators_less_verbosely)
{
	auto op = make_operator(4, '+', 7);
}

TEST_F(OperatorTest, can_make_aditions)
{
	auto op = make_operator(4, '+', 7);
	ASSERT_EQ(evaluate_node<int>(*op), 4 + 7);

	auto op2 = make_operator(4.3f, '+', 7);
	ASSERT_EQ(evaluate_node<float>(*op2), 4.3f + 7);
}

TEST_F(OperatorTest, can_make_substractions)
{
	auto op = make_operator(4, '-', 7);
	ASSERT_EQ(evaluate_node<int>(*op), 4 - 7);

	auto op2 = make_operator(4, '-', 7.76f);
	ASSERT_EQ(evaluate_node<float>(*op2), 4 - 7.76f);
}

TEST_F(OperatorTest, can_make_multiplications)
{
	auto op = make_operator(4, '*', 7);
	ASSERT_EQ(evaluate_node<int>(*op), 4 * 7);

	auto op2 = make_operator(4.2f, '*', 7.3f);
	ASSERT_EQ(evaluate_node<float>(*op2), 4.2f * 7.3f);
}

TEST_F(OperatorTest, can_make_divisions)
{
	auto op = make_operator(18, '/', 3);
	ASSERT_EQ(evaluate_node<int>(*op), 18 / 3);

	auto op2 = make_operator(1, '/', 3.46f);
	ASSERT_EQ(evaluate_node<float>(*op2), 1 / 3.46f);
}

TEST_F(OperatorTest, executes_all_calls_in_the_tree)
{
	auto lhs = make_operator(0.2f, '+', 0.8f);
	auto rhs = make_operator(4, '-', 0.34f);
	auto op = make_operator('/');
	op->set_operands(std::move(lhs), std::move(rhs));
	ASSERT_EQ(evaluate_node<float>(*op), (0.2f + 0.8f) / (4 - 0.34f));
}

class NamedVariableTest : public Test 
{
public:
	runtime::DispatchEngine eng;

};

TEST_F(NamedVariableTest, named_variable_requests_for_the_variable_to_the_engine)
{
	ASSERT_EQ(eng.get_variable_num(), 0u);

	// true because we want to create it
	const NamedVariable request{ "a", true };
	request.evaluate(eng);

	ASSERT_EQ(eng.get_variable_num(), 1u);

	const NamedVariable other_request{ "a" };
	other_request.evaluate(eng);

	ASSERT_EQ(eng.get_variable_num(), 1u);
}

TEST_F(NamedVariableTest, if_a_variable_already_exists_trows)
{
	try
	{
		// try to create two variables with the same name
		const NamedVariable request{ "a", true };
		request.evaluate(eng);
		request.evaluate(eng);

		FAIL();
	}
	catch (...) {}
}
