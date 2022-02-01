
#include "gmock\gmock.h"
using namespace testing;

#include "Runtime\Stack.h"
using namespace runtime;

#include "Runtime\RuntimeException.h"

class StackTest : public Test
{
public:
	Stack stk;
};

TEST_F(StackTest, stack_is_empty_by_default)
{
	ASSERT_EQ(stk.get_var_num(), 0u);
}
TEST_F(StackTest, stack_can_hold_variables)
{
	stk.create_variable("a");
	ASSERT_EQ(stk.get_var_num(), 1u);

	stk.create_variable("aa");
	ASSERT_EQ(stk.get_var_num(), 2u);
}
TEST_F(StackTest, stack_returns_the_boxed_value_that_represents_it)
{
	BoxedValue & a = stk.create_variable("a");
	a = BoxedValue{ 4 };

	ASSERT_EQ(boxed_cast<int>(*stk.get_variable("a")), 4);
}
TEST_F(StackTest, stack_can_create_a_variable_passing_its_value)
{
	stk.create_variable("a", BoxedValue{ 2.74f });
	const BoxedValue & a = *stk.get_variable("a");

	ASSERT_EQ(boxed_cast<float>(a), 2.74f);
}
TEST_F(StackTest, stack_returns_nullptr_when_trying_to_get_a_non_existing_variable)
{
	ASSERT_EQ(stk.get_variable("a"), nullptr);
}
TEST_F(StackTest, stack_throws_when_trying_to_createa_variable_that_already_exists_in_the_scope)
{
	stk.create_variable("a");

	try
	{
		stk.create_variable("a");
		FAIL();
	}
	catch (const except::RuntimeException &)
	{
		SUCCEED();
	}
}
TEST_F(StackTest, stack_can_create_new_scopes)
{
	stk.push_new_scope();

	stk.create_variable("a", BoxedValue{ 2.74f });
	ASSERT_EQ(boxed_cast<float>(*stk.get_variable("a")), 2.74f);

	stk.pop_scope();

	ASSERT_EQ(stk.get_variable("a"), nullptr);
}
TEST_F(StackTest, stack_acceses_variables_from_outer_scopes_if_the_variable_isnt_in_the_current_one)
{
	stk.create_variable("a", BoxedValue{ 2.74f });

	// variable is accessible 
	stk.push_new_scope();
	ASSERT_EQ(boxed_cast<float>(*stk.get_variable("a")), 2.74f);
	stk.pop_scope();

	ASSERT_EQ(boxed_cast<float>(*stk.get_variable("a")), 2.74f);
}
TEST_F(StackTest, stack_scopes_occlude_outer_variables)
{
	stk.create_variable("a", BoxedValue{ 2.74f });

	// variable is accessible 
	stk.push_new_scope();
	{
		stk.create_variable("a", BoxedValue{ false });
		ASSERT_EQ(boxed_cast<bool>(*stk.get_variable("a")), false);
	}
	stk.pop_scope();

	ASSERT_EQ(boxed_cast<float>(*stk.get_variable("a")), 2.74f);
}

