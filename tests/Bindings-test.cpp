
#include "gmock\gmock.h"
using namespace testing;

#include "Runtime\Bindings.h"
#include "Runtime\DispatchEngine.h"

class GlobalFunctionBindingTest : public Test
{
public:
	static bool global_fn()
	{
		return true;
	}
	static int pow_fn(int x, int p)
	{
		int result = 1;
		for (int i = 0; i < p; ++i)
			result *= x;
		return result;
	}
	static int * assign_value(int & a, int b)
	{
		a = b;
		return &a;
	}

	static const char * do_something_strings(const char * str)
	{
		return str;
	}

	static void dummy(bool & b)
	{
		b = true;
	}

	runtime::DispatchEngine engine;
};
TEST_F(GlobalFunctionBindingTest, can_call_global_funtion_bindings)
{
	const binds::GlobalFnBinding<bool> binded_fn{ global_fn };
	std::vector<BoxedValue> args{};
	ASSERT_TRUE(boxed_cast<bool>(binded_fn.do_call(engine, args)));
}
TEST_F(GlobalFunctionBindingTest, can_pass_parametters_to_global_functions)
{
	const binds::GlobalFnBinding<int, int, int> binded_fn{ pow_fn };
	std::vector<BoxedValue> args{ BoxedValue{ 2 }, BoxedValue{ 5 } };
	ASSERT_EQ(boxed_cast<int>(binded_fn.do_call(engine, args)), 32);
}
TEST_F(GlobalFunctionBindingTest, binding_can_conver_from_related_built_in_types)
{
	const binds::GlobalFnBinding<int *, int &, int> binded_fn{ assign_value };
	BoxedValue param{ 2 };
	std::vector<BoxedValue> args{ make_ref(param), BoxedValue{ 7 } };

	const BoxedValue bv = binded_fn.do_call(engine, args);
	ASSERT_EQ(&boxed_cast<int>(bv), &boxed_cast<int>(param));
	ASSERT_EQ(boxed_cast<int>(bv), boxed_cast<int>(param));
	ASSERT_EQ(boxed_cast<int>(param), 7);
}
TEST_F(GlobalFunctionBindingTest, binding_can_conver_to_diferent_types_of_strings)
{
	const binds::GlobalFnBinding<const char *, const char *> binded_fn{ do_something_strings };
	std::vector<BoxedValue> args{ BoxedValue{ std::string{ "some_string" } } };

	const BoxedValue bv = binded_fn.do_call(engine, args);
	ASSERT_EQ(boxed_cast<std::string>(bv), std::string{ "some_string" });
}
TEST_F(GlobalFunctionBindingTest, binding_can_hold_lambdas)
{
	const binds::GlobalFnBinding<int, int, int> binded_fn{
		std::function<int(int, int)>{ [](int a, int b) { return a + b; } }
	};
	std::vector<BoxedValue> args{ BoxedValue{ 4 }, BoxedValue{ 3 } };

	const BoxedValue bv = binded_fn.do_call(engine, args);
	ASSERT_EQ(boxed_cast<int>(bv), 3 + 4);
}
TEST_F(GlobalFunctionBindingTest, provides_interface_to_bind_functions)
{
	const auto binded_fn = binds::func(pow_fn);
	std::vector<BoxedValue> args{ BoxedValue{ 4 }, BoxedValue{ 3 } };

	const BoxedValue bv = binded_fn->do_call(engine, args);
	ASSERT_EQ(boxed_cast<int>(bv), std::pow(4, 3));
}
TEST_F(GlobalFunctionBindingTest, can_bind_functions_that_return_void)
{
	// we just want this to compile
	const auto binded_fn = binds::func(dummy);

	bool called = false;
	std::vector<BoxedValue> args{ BoxedValue{ &called } };
	binded_fn->do_call(engine, args);

	ASSERT_TRUE(called);
}

class GlobalVariablesBindingTest : public Test
{
public:
	static int global_int;
};
int GlobalVariablesBindingTest::global_int = 0;
TEST_F(GlobalVariablesBindingTest, can_bind_global_variables)
{
	global_int = 10;

	auto binded_var = binds::var(global_int);
	BoxedValue bv = binded_var.get_variable();

	ASSERT_EQ(global_int, 10);
	ASSERT_EQ(boxed_cast<int>(bv), 10);
	ASSERT_EQ(&boxed_cast<int>(bv), &global_int);
}
TEST_F(GlobalVariablesBindingTest, binded_variable_and_cpp_variable_change_at_the_same_type)
{
	global_int = 30;

	auto binded_var = binds::var(global_int);
	BoxedValue bv = binded_var.get_variable();

	ASSERT_EQ(global_int, 30);
	ASSERT_EQ(boxed_cast<int>(bv), 30);

	global_int = 32;
	ASSERT_EQ(boxed_cast<int>(bv), 32);

	boxed_cast<int>(bv) = 42;
	ASSERT_EQ(global_int, 42);
}
TEST_F(GlobalVariablesBindingTest, provides_interface_to_bind_variables)
{
	global_int = 30;
	auto binded_global = binds::var(global_int);
	ASSERT_EQ(boxed_cast<int>(binded_global.get_variable()), 30);

	global_int = 24;
	ASSERT_EQ(boxed_cast<int>(binded_global.get_variable()), 24);
}


class MemberFunctionBindingTest : public Test
{
public:
	runtime::DispatchEngine engine;
};
TEST_F(MemberFunctionBindingTest, can_call_a_binded_function)
{
	using FunctionType = binds::ConstMemberFnBinding<std::vector<BoxedValue>, std::size_t>;

	const FunctionType binded_fn{ &std::vector<BoxedValue>::size };

	BoxedValue inst{ std::vector<BoxedValue>{} };

	std::vector<BoxedValue> args{};
	ASSERT_EQ(boxed_cast<std::size_t>(binded_fn.do_call(engine, inst, args)), 0u);

	boxed_cast<std::vector<BoxedValue>>(inst).resize(6);
	ASSERT_EQ(boxed_cast<std::size_t>(binded_fn.do_call(engine, inst, args)), 6u);
}
TEST_F(MemberFunctionBindingTest, can_pass_parametters_to_a_binded_function)
{
	using FunctionType = binds::MemberFnBinding<std::vector<BoxedValue>, void, BoxedValue &&>;

	BoxedValue inst{ std::vector<BoxedValue>{} };
	const FunctionType binded_fn{ &std::vector<BoxedValue>::push_back };

	std::vector<BoxedValue> args{ BoxedValue{ std::string{ "Hey!!" } } };
	binded_fn.do_call(engine, inst, args);

	auto & v = boxed_cast<std::vector<BoxedValue>>(inst);
	ASSERT_EQ(v.size(), 1u);
	ASSERT_EQ(boxed_cast<std::string>(v.front()), std::string{ "Hey!!" });

	args.front() = BoxedValue{ 3 };
	binded_fn.do_call(engine, inst, args);

	ASSERT_EQ(boxed_cast<int>(v.back()), 3);
}
TEST_F(MemberFunctionBindingTest, provides_interface_to_create_bindigs_less_verbosely)
{
	// const function test
	{
		auto p_bind = binds::func(&std::vector<BoxedValue>::size);

		BoxedValue inst{ std::vector<BoxedValue>(6) };
		std::vector<BoxedValue> args{};

		ASSERT_EQ(boxed_cast<std::size_t>(p_bind->do_call(engine, inst, args)), 6u);
	}

	// non const function test
	{
		auto p_bind = binds::func<std::vector<BoxedValue>, void, BoxedValue&&>(&std::vector<BoxedValue>::push_back);

		BoxedValue inst{ std::vector<BoxedValue>{} };
		std::vector<BoxedValue> args{ BoxedValue{ std::string{ "foo" } } };
		p_bind->do_call(engine, inst, args);

		auto & v = boxed_cast<std::vector<BoxedValue>>(inst);
		ASSERT_EQ(v.size(), 1);
		ASSERT_EQ(boxed_cast<std::string>(v.front()), std::string{ "foo" });
	}
}
