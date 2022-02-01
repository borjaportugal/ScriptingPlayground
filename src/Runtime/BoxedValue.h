#pragma once

#include "Runtime/TypeInfo.h"

#include <vector>	// std::vector
#include <typeinfo>	// std::type_info
#include <memory>	// std::unique_ptr, std::make_unique
#include <type_traits>	// std::enable_if_t, std::is_arithmetic, std::remove_pointer_t, std::remove_reference_t

class BadBoxedCast : public std::exception
{
public:
	BadBoxedCast(const std::type_info & from, const std::type_info & to);
	const char * what() const noexcept override { return m_what.c_str(); }

private:
	std::string m_what;
};

struct BoxedValueStoreRef_t {};

template <typename T, std::size_t N>
class inline_unique_ptr;

namespace impl
{
	template <typename T, std::size_t N>
	struct inline_unique_ptr_deleter
	{
		constexpr static std::size_t buffer_size = N;

		void operator()(T * ptr) const
		{
			// TODO(Borja): uncomment this
			//ptr->~T();
			if (!is_inlined(ptr))
				::operator delete(static_cast<void *>(ptr));
		}

		bool is_inlined(void * ptr) const { return static_cast<const void *>(m_buffer) == ptr; }

		unsigned char m_buffer[N];
	};

	template <typename T, std::size_t N, bool>
	struct make_inline_unique_ptr_impl
	{
		template <typename ... Ts>
		static inline_unique_ptr<T, N> make(Ts && ... vs)
		{
			return inline_unique_ptr<T, N>{ new T(std::forward<Ts>(vs) ...) };
		}
	};

	template <typename T, std::size_t N>
	struct make_inline_unique_ptr_impl<T, N, true>
	{
		template <typename ... Ts>
		static inline_unique_ptr<T, N> make(Ts && ... vs)
		{
			inline_unique_ptr<T, N> ptr;
			ptr.reset(::new (static_cast<void *>(ptr.get_deleter().m_buffer)) T(std::forward<Ts>(vs) ...));
			ptr.m_inlined = true;
			return ptr;
		}
	};

	template <typename T, std::size_t N>
	using make_inline_unique_ptr = make_inline_unique_ptr_impl<T, N, sizeof(T) <= N>;
}

template <typename T, std::size_t N>
class inline_unique_ptr
	: public std::unique_ptr<T, ::impl::inline_unique_ptr_deleter<T, N>>
{
public:
	constexpr static std::size_t buffer_size = N;
	using Deleter = ::impl::inline_unique_ptr_deleter<T, N>;
	using Base = std::unique_ptr<T, Deleter>;

	template <typename T, std::size_t N, bool>
	friend struct ::impl::make_inline_unique_ptr_impl;

public:
	inline_unique_ptr() = default;
	inline_unique_ptr(std::nullptr_t) : Base{ nullptr } {}
	template <typename U>
	explicit inline_unique_ptr(U * val) : Base{ val } {}
	template <typename U>
	inline_unique_ptr(inline_unique_ptr<U, N> && other)
		: Base{ nullptr }
	{
		construct_from(std::move(other));
	}
	template <typename U>
	inline_unique_ptr<T, N> & operator=(inline_unique_ptr<U, N> && other)
	{
		if (this != &other)
			construct_from(std::move(other));
		return *this;
	}

	bool is_inlined() const
	{
		//return get_deleter().is_inlined(get());
		return m_inlined;
	}

private:
	template <typename U>
	void construct_from(inline_unique_ptr<U, N> && other)
	{
		if (other.is_inlined())
		{
			auto * obj = reinterpret_cast<T *>(get_deleter().m_buffer);
			reset(obj);
			std::memcpy(obj, other.release(), Deleter::buffer_size);
			m_inlined = true;
		}
		else
		{
			reset(other.release());
			m_inlined = false;
		}
	}

	bool m_inlined{ false };
};

template <typename T, std::size_t N, typename ... Ts>
static inline_unique_ptr<T, N> make_inline_unique_ptr(Ts && ... vs)
{
	return ::impl::make_inline_unique_ptr<T, N>::make(std::forward<Ts>(vs) ...);
}

/// \brief	Stores any value of the script, this allows dynamic variable typing 
///			(i.e. change the type of a variable while the script is running).
class BoxedValue
{
private:
	struct IValue;
#if 0
	// TODO(Borja): inline_unique_ptr is not working, crashes
	using value_ptr = inline_unique_ptr<IValue, sizeof(void*) * 4>;

	template <typename T, typename ... Ts>
	static auto make_value(Ts && ... vs)
	{
		return make_inline_unique_ptr<Value<T>, value_ptr::buffer_size>(std::forward<Ts>(vs) ...);
	}
#else
	using value_ptr = std::unique_ptr<IValue>;
	template <typename T, typename ... Ts>
	static auto make_value(Ts && ... vs)
	{
		return std::make_unique<Value<T>>(std::forward<Ts>(vs) ...);
	}
#endif

	template <typename T, bool B>
	using enable_if_arithmetic_impl_t = std::enable_if_t<B == std::is_arithmetic<std::decay_t<T>>::value>;

	template <typename T>
	using enable_if_arithmetic_t = enable_if_arithmetic_impl_t<T, true>;
	template <typename T>
	using enable_if_not_arithmetic_t = enable_if_arithmetic_impl_t<T, false>;
	template <typename T>
	using enable_if_not_BoxedValue_t = std::enable_if_t<!std::is_same<BoxedValue, std::decay_t<T>>::value>;

	struct IValue
	{
		virtual ~IValue() = default;
		IValue() = default;
		IValue(IValue &&) = default;
		IValue(const IValue &) = default;
		IValue& operator=(IValue &&) = default;
		IValue& operator=(const IValue &) = default;

		virtual const TypeInfo & get_type_info() const = 0;
		virtual value_ptr clone() const = 0;
	};

	template <typename T>
	struct ValueTraits_impl : public IValue
	{
		using get_value_return_t = T;
		virtual T & get_value() = 0;
		virtual const T & get_value() const = 0;
	};

	template <typename T>
	using ValueTraits = ValueTraits_impl<std::remove_const_t<T>>;

	template <typename T>
	struct Value final
		: public ValueTraits<T>
	{
		Value() = default;
		Value(const Value & other) : m_v{ other.m_v } {}

		Value(const T & t) : m_v(t) {}
		template <typename = enable_if_not_arithmetic_t<T>>
		Value(T && t) : m_v(std::forward<T>(t)) {}

		const TypeInfo & get_type_info() const override { return ::get_type_info<T>(); }

		value_ptr clone() const override { return make_value<T>(*this); }

		get_value_return_t & get_value() override { return m_v; }
		const get_value_return_t & get_value() const override { return m_v; }

		T m_v;
	};

public:
	BoxedValue() = default;
	BoxedValue(const BoxedValue & other);
	BoxedValue& operator=(const BoxedValue & rhs);
	BoxedValue(BoxedValue &&) = default;
	BoxedValue& operator=(BoxedValue &&) = default;

	template <typename T,
		typename = enable_if_not_arithmetic_t<T>,
		typename = enable_if_not_BoxedValue_t<T>
	>
		explicit BoxedValue(T && t)
		: m_boxed_value{ make_value<std::remove_reference_t<T>>(std::forward<T>(t)) }
	{}

	template <typename T, typename = enable_if_arithmetic_t<T>>
	explicit BoxedValue(T t)
		: m_boxed_value{ make_value<std::decay_t<T>>(t) }
	{}

	template <typename T>
	BoxedValue(BoxedValueStoreRef_t, T & t)
		: m_boxed_value{ make_value<T *>(&t) }
	{}

	inline const TypeInfo & get_type_info() const { return get_value().get_type_info(); }

	// STUDY(Borja): instead of dynamic casting storing the typeid and then comparing it with
	// this types may speed up things.
	// We could also store flags about the type, is it a pointer? reference? ...
	template <typename T>
	T & get_as()
	{
		if (auto val = dynamic_cast<ValueTraits<T > *>(&get_value()))
			return val->get_value();
		if (auto val = dynamic_cast<ValueTraits<T *> *>(&get_value()))
			return *val->get_value();
		if (auto val = dynamic_cast<ValueTraits<T &> *>(&get_value()))
			return val->get_value();
		if (auto val = dynamic_cast<ValueTraits<std::shared_ptr<T>> *>(&get_value()))
			return *val->get_value();

		throw BadBoxedCast{ get_type_info().get_bare_std_type_info(), typeid(T) };
	}
	template <typename T>
	const T & get_as() const
	{
		if (auto val = dynamic_cast<const ValueTraits<T  > *>(&get_value()))
			return val->get_value();
		if (auto val = dynamic_cast<const ValueTraits<T *> *>(&get_value()))
			return *val->get_value();
		if (auto val = dynamic_cast<const ValueTraits<T &> *>(&get_value()))
			return val->get_value();
		if (auto val = dynamic_cast<const ValueTraits<std::shared_ptr<T>> *>(&get_value()))
			return *val->get_value();

		throw BadBoxedCast{ get_type_info().get_bare_std_type_info(), typeid(T) };
	}

	template <typename T>
	T get_exactly_as()
	{
		if (auto val = dynamic_cast<Value<T> *>(&get_value()))
			return val->m_value;

		throw BadBoxedCast{ get_type_info().get_std_type_info(), typeid(T) };
	}
	template <typename T>
	T get_exactly_as() const
	{
		if (auto val = dynamic_cast<const Value<T> *>(&get_value()))
			return val->m_value;

		throw BadBoxedCast{ get_type_info().get_std_type_info(), typeid(T) };
	}

	bool empty() const { return !m_boxed_value.operator bool(); }

	///	\return true if 'this' BoxedValue stores a T, T& or T*
	template <typename T>
	bool is_storing() const { return !empty() && get_type_info().bare_equal(::get_type_info<T>()); }
	template <typename T>
	bool is_storing_exactly() const { return !empty() && get_type_info().equal(::get_type_info<T>()); }

private:
	IValue & get_value() { return *m_boxed_value; }
	const IValue & get_value() const { return *m_boxed_value; }

	value_ptr m_boxed_value{ nullptr };
};

template <typename T>
T & boxed_cast(BoxedValue & bv) { return bv.get_as<T>(); }
template <typename T>
const T & boxed_cast(const BoxedValue & bv) { return bv.get_as<T>(); }

// STUDY(Borja): in the future problems may arise when we need to return a const reference...
/// \brief	Returns a BoxedValue containing a reference to the input one.
///			The returned BoxedValue stores a BoxedValue * to the input one, 
///			that is how the refernce works.
inline BoxedValue make_ref(BoxedValue & bv) { return BoxedValue{ &bv }; }

///	\brief	Checks if the input boxed value has a reference stored in it, if so returns it,
///			if it doesn't just returns the input one 'bv'
BoxedValue & resolve_ref(BoxedValue & bv);
const BoxedValue & resolve_ref(const BoxedValue & bv);

template <typename T>
T & resolve_ref_cast(BoxedValue & bv)
{
	return boxed_cast<T>(resolve_ref(bv));
}
template <typename T>
const T & resolve_ref_cast(const BoxedValue & bv)
{
	return boxed_cast<T>(resolve_ref(bv));
}

