/*!
\author Borja Portugal Martin
\brief	This is a C++14 implementation of what a C++17 constexpr if.
		This implementation is the one that Vitorio Romero proposes in his talk
		at cppcon2016 https://youtu.be/aXSsUqVSe2k
*/

#pragma once

#include <type_traits>	// std::is_same
#include <utility>		// std::forward

namespace meta
{
	namespace detail
	{
		template <typename Pred>
		auto make_static_if();

		template <typename F>
		class static_if_result
		{
		public:
			explicit static_if_result(F && f) : mFn(std::forward<F>(f)) {}

			// we already have the function we need to call, ignore all calls to this ones
			template <typename F>
			auto else_(F &&) const { return *this; }
			template <typename Pred>
			auto else_if(Pred&&) const { return *this; }
			template <typename F>
			const static_if_result & then(F &&) const { return *this; }

			// call to the function we need to call
			template <typename ... Args>
			auto operator()(Args && ... vs) const
			{
				return mFn(std::forward<Args>(vs)...);
			}

		private:
			F mFn;
		};

		/*!
		\brief	General case for the static if (false) this branch doesn't have to be picked.
		*/
		template <bool>
		class static_if_impl
		{
		public:
			// ignore the branch
			template <typename F>
			const static_if_impl & then(F &&) const { return *this; }

			template <typename Pred>
			auto else_if(Pred&&) const
			{
				return make_static_if<Pred>();
			}

			template <typename F>
			static_if_result<F> else_(F && f) const
			{
				return static_if_result<F>{ std::forward<F>(f) };
			}
		};

		/*!
		\brief	Case for when the branch is the one that needs to be picked.
		*/
		template <>
		class static_if_impl<true>
		{
		public:
			template <typename F>
			static_if_result<F> then(F && f) const
			{
				return static_if_result<F>{ std::forward<F>(f) };
			}
		};

		template <typename Pred>
		auto make_static_if()
		{
			return static_if_impl < static_cast<bool>(Pred{}) > {};
		}
	}

	template <typename Pred>
	auto static_if(Pred)
	{
		static_assert(!std::is_same<Pred, bool>::value, "The predicate needs to be an integral constant.");
		return detail::make_static_if<Pred>();
	}
}

