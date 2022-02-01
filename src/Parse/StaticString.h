#pragma once

#include <cstddef>	// std::size_t
#include <iostream>

namespace parse
{
	class StaticString
	{
	public:
		template <std::size_t N>
		constexpr StaticString(const char(&str)[N])
			: m_str(str)
			, m_size(N - 1)
		{}

		inline bool same_beggining(const char * str) const
		{
			for (unsigned i = 0; i < size(); ++i)
			{
				if (m_str[i] != str[i])	return false;
			}
			return true;
		}


		constexpr char operator[](std::size_t i) const { return m_str[i]; }
		constexpr const char * c_str() const { return m_str; }
		constexpr std::size_t length() const { return size(); }
		constexpr std::size_t size() const { return m_size; }

	private:
		const char * const m_str;
		const std::size_t m_size;
	};
}

inline std::ostream & operator<<(std::ostream & os, const parse::StaticString & str)
{
	os << str.c_str();
	return os;
}
