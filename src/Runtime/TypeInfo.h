
#pragma once

#include <typeinfo>

/// \brief	Stores the necessary data that the scripting engine needs for the types.
class TypeInfo
{
public:
	using id_type = unsigned short;
	static constexpr id_type invalid_id{ static_cast<id_type>(~static_cast<id_type>(0)) };
private:
	TypeInfo(id_type id,
		const std::type_info & type_info,
		const std::type_info & bare_type_info)
		: m_unique_id{ id }
		, m_type_info{ &type_info }
		, m_bare_type_info{ &bare_type_info }
	{}

	template <typename T>
	friend const TypeInfo & get_type_info();

public:
	TypeInfo() = default;
	TypeInfo(const TypeInfo &) = default;
	TypeInfo(TypeInfo &&) = default;
	TypeInfo& operator=(const TypeInfo &) = default;
	TypeInfo& operator=(TypeInfo &&) = default;
	
	bool equal(const TypeInfo & rhs) const { return m_type_info == rhs.m_type_info; }
	bool bare_equal(const TypeInfo & rhs) const { return m_bare_type_info == rhs.m_bare_type_info; }

	bool empty() const { return m_unique_id == invalid_id; }

	id_type get_unique_id() const { return m_unique_id; }
	const std::type_info & get_std_type_info() const { return *m_type_info; }
	const std::type_info & get_bare_std_type_info() const { return *m_bare_type_info; }
	
private:
	id_type m_unique_id{ invalid_id };
	const std::type_info * m_type_info{ nullptr };
	const std::type_info * m_bare_type_info{ nullptr };
};

inline bool operator==(const TypeInfo & lhs, const TypeInfo & rhs)
{
	return lhs.get_unique_id() == rhs.get_unique_id();
}
inline bool operator!=(const TypeInfo & lhs, const TypeInfo & rhs)
{
	return !operator==(lhs, rhs);
}

inline bool operator==(const TypeInfo & lhs, const std::type_info & rhs)
{
	return lhs.get_std_type_info() == rhs;
}
inline bool operator!=(const TypeInfo & lhs, const std::type_info & rhs)
{
	return !operator==(lhs, rhs);
}

#include <memory>

namespace impl
{
	TypeInfo::id_type generate_unique_id();

	template <typename T>
	TypeInfo::id_type unique_id_for()
	{
		static const auto id = generate_unique_id();
		return id;
	}

	template <typename T>
	struct BareType_impl { using type = T; };

	template <typename T>
	struct BareType_impl<T *> : BareType_impl<T> {};

	template <typename T>
	struct BareType_impl<T &> : BareType_impl<T> {};

	template <typename T>
	struct BareType_impl<const T> : BareType_impl<T> {};

	template <typename T>
	struct BareType_impl<std::shared_ptr<T>> : BareType_impl<T> {};

	template <typename T>
	using bare_type_t = typename BareType_impl<T>::type;
}

template <typename T>
const TypeInfo & get_type_info()
{
	using bare_type = ::impl::bare_type_t<T>;
	static const TypeInfo s_type_info{ ::impl::unique_id_for<bare_type>(), typeid(T), typeid(bare_type) };
	return s_type_info;
}

using type_pair_key = unsigned long;
inline type_pair_key get_type_pair_hash(const TypeInfo & a, const TypeInfo & b)
{
	static_assert(sizeof(type_pair_key) >= sizeof(TypeInfo::id_type) * 2, "");

	static constexpr auto shift_bits = (sizeof(unsigned long) * 8) / 2;
	return (static_cast<type_pair_key>(a.get_unique_id()) << shift_bits) | b.get_unique_id();
}

template <typename T1, typename T2>
type_pair_key get_type_pair_hash()
{
	static const auto hash = get_type_pair_hash(get_type_info<T1>(), get_type_info<T2>());
	return hash;
}
