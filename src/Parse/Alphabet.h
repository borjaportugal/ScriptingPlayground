#pragma once

#include "StaticString.h"

#include <array>	// std::array
#include <initializer_list>	// std::initializer_list

namespace parse
{
	/// \brief	Stores a series of characters for latters checking if they belong to it or not.
	class Alphabet
	{
	private:
		static const std::size_t ALPHABET_SIZE = 256u;
		using alphabet_type = std::array<bool, ALPHABET_SIZE>;

	public:
		Alphabet(const std::initializer_list<unsigned char> & cs)
		{
			mAlphabet.fill(false);
			for (const auto & c : cs)
				mAlphabet[c] = true;
		}
		Alphabet(const std::initializer_list<std::pair<unsigned char, unsigned char>> & ranges)
		{
			mAlphabet.fill(false);
			for (const auto & range : ranges)
			{
				for (unsigned i = range.first; i <= range.second; ++i)
					mAlphabet[i] = true;
			}
		}

		inline bool contains(unsigned char c) const { return mAlphabet[c]; }
		static constexpr std::size_t size() { return ALPHABET_SIZE; }

	private:
		alphabet_type mAlphabet;
	};
}
