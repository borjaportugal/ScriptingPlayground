
#include "gmock\gmock.h"
using namespace testing;

#include "Parse\Alphabet.h"

TEST(AlphabetTest, alphabet_does_not_contain_anything_by_default)
{
	const parse::Alphabet a{ { 0 } };

	for (unsigned i = 1; i < a.size(); ++i)
		ASSERT_FALSE(a.contains(static_cast<unsigned char>(i)));
}
TEST(AlphabetTest, alphabet_can_have_specific_characters)
{
	const parse::Alphabet a{ '(', '|', '\"' };

	ASSERT_TRUE(a.contains('('));
	ASSERT_TRUE(a.contains('|'));
	ASSERT_TRUE(a.contains('\"'));

	ASSERT_FALSE(a.contains('9'));
	ASSERT_FALSE(a.contains('!'));
}
TEST(AlphabetTest, alphabet_can_have_range_characters)
{
	const parse::Alphabet a{ { 'a', 'z' }, {'A', 'Z' } };

	for (unsigned i = 0; i < a.size(); ++i)
	{
		if (i >= 'a' && i <= 'z')
			ASSERT_TRUE(a.contains(static_cast<unsigned char>(i)));
		else if (i >= 'A' && i <= 'Z')
			ASSERT_TRUE(a.contains(static_cast<unsigned char>(i)));
		else
			ASSERT_FALSE(a.contains(static_cast<unsigned char>(i)));
	}
}
