
#include "gmock\gmock.h"
using namespace testing;

#include "Runtime\BoxedValue.h"

#include <string>	// std::string

class BoxedValueTest : public Test {};

TEST_F(BoxedValueTest, boxed_value_provides_interface_to_know_if_is_storing_something)
{
	const BoxedValue bv0;
	ASSERT_TRUE(bv0.empty());

	const BoxedValue bv1{ 4 };
	ASSERT_FALSE(bv1.empty());
}
TEST_F(BoxedValueTest, can_ask_to_a_boxed_value_for_the_type_of_the_value_is_storing)
{
	const BoxedValue bv0{ 4 };
	ASSERT_EQ(bv0.get_type_info(), typeid(int));

	const BoxedValue bv1{ 4.2f };
	ASSERT_EQ(bv1.get_type_info(), typeid(float));

	const BoxedValue bv2{ false };
	ASSERT_EQ(bv2.get_type_info(), typeid(bool));
}
TEST_F(BoxedValueTest, can_ca_boxed_value)
{
	const BoxedValue bv0{ 4 };
	ASSERT_EQ(bv0.get_as<int>(), 4);

	const BoxedValue bv1{ std::string{ "Some very cool string" } };
	ASSERT_TRUE(boxed_cast<std::string>(bv1) == "Some very cool string");
}
TEST_F(BoxedValueTest, boxed_value_throws_on_invalid_cast)
{
	const BoxedValue bv{ 4 };

	try
	{
		boxed_cast<std::string>(bv);
		FAIL();
	}
	catch (const BadBoxedCast &) {}
	catch (...) 
	{
		FAIL();
	}

	try
	{
		boxed_cast<std::string>(bv);
		FAIL();
	}
	catch (const std::exception &) {}
	catch (...) 
	{
		FAIL();
	}
}
TEST_F(BoxedValueTest, boxed_values_can_store_references_to_other_boxed_values)
{
	BoxedValue bv{ 4 };
	BoxedValue bv_ref = make_ref(bv);

	ASSERT_NE(&bv_ref, &bv);				// they are different BoxedValues
	ASSERT_EQ(&resolve_ref(bv_ref), &bv);	// reference is the same

	// resolve_ref returns the same bv if it isn't storing a reference
	ASSERT_EQ(&resolve_ref(bv), &bv);
}
TEST_F(BoxedValueTest, boxed_value_references_have_same_value_as_referenced_bv)
{
	BoxedValue bv{ 4 };
	const BoxedValue bv_ref = make_ref(bv);

	ASSERT_EQ(boxed_cast<int>(bv), 4);
	ASSERT_EQ(boxed_cast<int>(resolve_ref(bv_ref)), boxed_cast<int>(bv));

	bv = BoxedValue{ 37 };
	ASSERT_EQ(boxed_cast<int>(resolve_ref(bv_ref)), boxed_cast<int>(bv));
}
TEST_F(BoxedValueTest, boxed_value_calls_correctly_to_the_objects_dtor)
{
	struct SmallObject
	{
		explicit SmallObject(int & i) : m_dtor_calls{ &i } {}
		~SmallObject() { *m_dtor_calls += 1; }

		int * m_dtor_calls;
	};
	struct BigObject
	{
		explicit BigObject(int & i) : m_dtor_calls{ &i } {}
		~BigObject() { *m_dtor_calls += 1; }

		int * m_dtor_calls;
		char m_big_object[2048];
	};

	int dtor_calls = 0;
	{
		BoxedValue bv{ SmallObject{ dtor_calls } };
		BoxedValue bv2{ BigObject{ dtor_calls } };
	}

	ASSERT_EQ(dtor_calls, 4);
}
