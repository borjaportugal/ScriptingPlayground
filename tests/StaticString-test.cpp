
#include "gmock\gmock.h"
using namespace testing;

#include "Parse\StaticString.h"
using namespace parse;

TEST(StaticStringTest, static_string_can_be_constructed_out_of_a_c_string)
{
	const StaticString str{ "test" };
	ASSERT_EQ(str.size(), 4u);
	ASSERT_EQ(str.length(), str.size());
}

TEST(StaticStringTest, static_string_can_check_if_other_string_starts_with_its_value)
{
	const StaticString str{ "test" };
	ASSERT_TRUE(str.same_beggining("test should return true"));
}

