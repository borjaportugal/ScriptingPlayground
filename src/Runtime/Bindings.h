#pragma once

#include "Forwards.h"	// runtime::DispatchEngine
#include "BoxedValue.h"
#include "static_if.h"
#include "RuntimeException.h"

#include <functional>	// std::function
#include <utility>		// std::integer_sequence

// function bindings
namespace binds
{
	namespace impl
	{
		enum class ResolutionType : unsigned 
		{
			NOT_CONVERTIBLE = 0,
			EXACT_MATCH = 1,
			CONVERSION_REQUIRED = 2,
		};

		template<typename T>
		struct ResolveParamTraits
		{
			using cast_type = std::remove_reference_t<std::remove_pointer_t<T>>;
			using type = cast_type;
			static type cast(BoxedValue & bv, runtime::DispatchEngine & en)
			{
				auto & resolved_bv = resolve_ref(bv);

				if (resolved_bv.is_storing<cast_type>())
					return resolved_bv.get_as<cast_type>();

				if (const auto * conv = en.get_type_conversion(resolved_bv.get_type_info(), get_type_info<T>()))
					return conv->convert<cast_type>(resolved_bv);

				SCR_RUNTIME_EXCEPTION("Cannot convert parameter of type '",
									  resolved_bv.get_type_info().get_bare_std_type_info().name(), "' to '",
									  typeid(cast_type).name(), "' to call function.");
			}

			static ResolutionType convertible(BoxedValue & bv, runtime::DispatchEngine & en)
			{
				auto & resolved_bv = resolve_ref(bv);

				if (resolved_bv.is_storing<cast_type>())
					return ResolutionType::EXACT_MATCH;

				const auto * conv = en.get_type_conversion(resolved_bv.get_type_info(), get_type_info<T>());
				return conv == nullptr ? ResolutionType::NOT_CONVERTIBLE : ResolutionType::CONVERSION_REQUIRED;
			}
		};
		template<typename T>
		struct ResolveParamTraits<T &>
		{
			using cast_type = std::remove_reference_t<std::remove_pointer_t<T>>;
			using type = cast_type &;
			static type & cast(BoxedValue & bv, runtime::DispatchEngine &)
			{
				auto & resolved_bv = resolve_ref(bv);

				if (resolved_bv.is_storing<cast_type>())
					return resolved_bv.get_as<cast_type>();

				SCR_RUNTIME_EXCEPTION("Cannot convert parameter of type '",
									  resolved_bv.get_type_info().get_bare_std_type_info().name(), "' to '",
									  typeid(cast_type).name(), " &' to call function.");
			}

			static ResolutionType convertible(const BoxedValue & bv, const runtime::DispatchEngine &)
			{
				return resolve_ref(bv).is_storing<cast_type>() ? ResolutionType::EXACT_MATCH : ResolutionType::NOT_CONVERTIBLE;
			}
		};

		template<>
		struct ResolveParamTraits<const char *>
		{
			using type = const char *;
			static const char * cast(BoxedValue & bv, runtime::DispatchEngine &)
			{
				return resolve_ref_cast<std::string>(bv).c_str();
			}

			static ResolutionType convertible(const BoxedValue & bv, const runtime::DispatchEngine &)
			{
				return resolve_ref(bv).is_storing<std::string>() ? ResolutionType::EXACT_MATCH : ResolutionType::NOT_CONVERTIBLE;
			}
		};
		template<>
		struct ResolveParamTraits<BoxedValue>
		{
			using type = BoxedValue;
			static BoxedValue & cast(BoxedValue & bv, runtime::DispatchEngine &) 
			{
				return resolve_ref(bv); 
			}
			static ResolutionType convertible(const BoxedValue & bv, const runtime::DispatchEngine &)
			{
				return bv.is_storing<type>() ? ResolutionType::EXACT_MATCH : ResolutionType::NOT_CONVERTIBLE;
			}
		};
		template<>
		struct ResolveParamTraits<BoxedValue &>
		{
			using type = BoxedValue &;
			static BoxedValue & cast(BoxedValue & bv, runtime::DispatchEngine &)
			{ 
				return resolve_ref(bv); 
			}
			
			static ResolutionType convertible(const BoxedValue & bv, const runtime::DispatchEngine &)
			{
				return bv.is_storing<type>() ? ResolutionType::EXACT_MATCH : ResolutionType::NOT_CONVERTIBLE;
			}
		};
		template<>
		struct ResolveParamTraits<BoxedValue &&>
		{
			using type = BoxedValue &;
			static BoxedValue & cast(BoxedValue & bv, runtime::DispatchEngine &) 
			{ 
				return resolve_ref(bv); 
			}
			static ResolutionType convertible(const BoxedValue & bv, const runtime::DispatchEngine &)
			{
				return bv.is_storing<type>() ? ResolutionType::EXACT_MATCH : ResolutionType::NOT_CONVERTIBLE;
			}
		};

		template <typename T>
		class ResolveParamType
		{
		public:
			ResolveParamType(BoxedValue & bv,
				runtime::DispatchEngine & engine)
				: m_bv{ bv }
				, m_engine{ engine }
			{}

			using traits = ResolveParamTraits<T>;
			using type = typename traits::type;
			operator type () { return traits::cast(m_bv, m_engine); }

			ResolutionType is_convertible() { return traits::convertible(m_bv, m_engine); }

		private:
			BoxedValue & m_bv;
			runtime::DispatchEngine & m_engine;
		};
		template <typename T>
		class ResolveParamType< T && >
		{
		public:
			ResolveParamType(BoxedValue & bv,
				runtime::DispatchEngine & engine)
				: m_bv{ bv }
				, m_engine{ engine }
			{}

			using traits = ResolveParamTraits<T>;
			using type = typename ResolveParamTraits<T>::type;
			operator T && () { return std::move(traits::cast(m_bv, m_engine)); }

			ResolutionType is_convertible() const { return traits::convertible(m_bv, m_engine); }

		private:
			BoxedValue & m_bv;
			runtime::DispatchEngine & m_engine;
		};

		template <typename T>
		struct ResolveReturnType_impl { using type = T; };
		template <>
		struct ResolveReturnType_impl<const char *> { using type = std::string; };

		template <typename T>
		using ResolveReturnType = typename ResolveReturnType_impl<T>::type;

		class FunctionCallMatchScore
		{
		public:
			static const int invalid = -1;
			FunctionCallMatchScore(int init_score) : m_score{ init_score } {}

			bool is_valid() const { return m_score > 0; }
			int get_score() const { return m_score; }

			inline FunctionCallMatchScore operator+=(ResolutionType type)
			{
				if (m_score == invalid)	return *this;

				switch (type)
				{
				case ResolutionType::NOT_CONVERTIBLE:
					m_score = invalid;
					break;
				case ResolutionType::EXACT_MATCH:
					m_score++;
					break;
				}

				return *this;
			}

			bool operator==(const FunctionCallMatchScore & rhs) const { return get_score() == rhs.get_score(); }
			bool operator!=(const FunctionCallMatchScore & rhs) const { return !operator==(rhs); }
			bool operator<(const FunctionCallMatchScore & rhs) const { return get_score() < rhs.get_score(); }
			bool operator>(const FunctionCallMatchScore & rhs) const { return get_score() > rhs.get_score(); }
			bool operator<=(const FunctionCallMatchScore & rhs) const { return operator==(rhs) || operator<(rhs); }
			bool operator>=(const FunctionCallMatchScore & rhs) const { return operator==(rhs) || operator>(rhs); }

		private:
			int m_score{ 0 };
		};

		template <typename ... Args, std::size_t ... Is>
		FunctionCallMatchScore get_function_call_score(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args,
			std::index_sequence<Is ...>)
		{
			(void)en;
			(void)args;

			if (sizeof...(Args) != args.size())
				return{ FunctionCallMatchScore::invalid };

			FunctionCallMatchScore score = 0;
			const std::initializer_list<int> are_convertible{
				(score += impl::ResolveParamType<Args>(args[Is], en).is_convertible(), 0) ...
			};

			return score;
		}
	}
	
	class IGlobalFunctionBinding
	{
	public:
		IGlobalFunctionBinding() = default;
		IGlobalFunctionBinding(IGlobalFunctionBinding &&) = default;
		IGlobalFunctionBinding& operator=(IGlobalFunctionBinding &&) = default;
		virtual ~IGlobalFunctionBinding() = default;

		virtual BoxedValue do_call(runtime::DispatchEngine & en, 
								   std::vector<BoxedValue> & args) const = 0;
	};

	class GlobalFunctionBinding 
		: public IGlobalFunctionBinding
	{
	public:
		virtual impl::FunctionCallMatchScore get_call_score(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args) const = 0;
	};
	
	class OverloadedGlobalFunctionBinding
		: public IGlobalFunctionBinding
	{
	public:
		OverloadedGlobalFunctionBinding(
			std::unique_ptr<GlobalFunctionBinding> && overload0,
			std::unique_ptr<GlobalFunctionBinding> && overload1);

		BoxedValue do_call(runtime::DispatchEngine & en,
				std::vector<BoxedValue> & args) const override;

		void add_overload(std::unique_ptr<GlobalFunctionBinding> && overload);

	private:
		std::vector<std::unique_ptr<GlobalFunctionBinding>> m_overloads;
	};

	template <typename R, typename ... Args>
	class GlobalFnBinding : public GlobalFunctionBinding
	{
	public:
		using fn_type = std::function<R(Args...)>;
		GlobalFnBinding(R(*fn)(Args ...))
			: m_fn(fn)
		{}
		GlobalFnBinding(fn_type && fn)
			: m_fn(std::move(fn))
		{}

		BoxedValue do_call(runtime::DispatchEngine & en,
						   std::vector<BoxedValue> & args) const override
		{
			if (sizeof...(Args) != args.size())
				SCR_RUNTIME_EXCEPTION("Invalid parameter number, expecting ", sizeof...(Args), " and received ", args.size());

			return meta::static_if(std::is_same < R, void>{})
				// returning void
				.then([this](auto & en, auto & args)
			{
				do_call_impl(en, args, std::index_sequence_for<Args ...>{});
				return BoxedValue{};
			})
				// returning parameter
				.else_([this](auto & en, auto & args)
			{
				return BoxedValue{ do_call_impl(en, args, std::index_sequence_for<Args ...>{}) };
			})(en, args);
		}
	private:
		impl::FunctionCallMatchScore get_call_score(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args) const override
		{
			return impl::get_function_call_score<Args...>(en, args, std::index_sequence_for<Args...>{});
		}

		template <std::size_t ... Is>
		impl::ResolveReturnType<R> do_call_impl(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args,
			std::index_sequence<Is ...>) const
		{
			(void)en;
			(void)args;

			return m_fn(impl::ResolveParamType<Args>(args[Is], en) ...);
		}

		fn_type m_fn;
	};

	class IMemberFunctionBinding
	{
	public:
		IMemberFunctionBinding() = default;
		IMemberFunctionBinding(IMemberFunctionBinding &&) = default;
		IMemberFunctionBinding& operator=(IMemberFunctionBinding &&) = default;
		virtual ~IMemberFunctionBinding() = default;

		virtual BoxedValue do_call(runtime::DispatchEngine & en, 
								   BoxedValue & inst, std::vector<BoxedValue> & args) const = 0;
	};

	class MemberFunctionBinding
		: public IMemberFunctionBinding
	{
	public:
		virtual impl::FunctionCallMatchScore get_call_score(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args) const = 0;
		virtual const TypeInfo & get_class_type_info() const = 0;
	};

	class OverloadedMemberFunctionBinding
		: public IMemberFunctionBinding
	{
	public:
		OverloadedMemberFunctionBinding(
			std::unique_ptr<MemberFunctionBinding> && overload0,
			std::unique_ptr<MemberFunctionBinding> && overload1);

		BoxedValue do_call(
			runtime::DispatchEngine & en, BoxedValue & inst,
			std::vector<BoxedValue> & args) const override;

		void add_overload(std::unique_ptr<MemberFunctionBinding> && overload);

	private:
		std::vector<std::unique_ptr<MemberFunctionBinding>> m_overloads;
	};

	template <typename T, typename FN, typename R, typename ... Args>
	class MemberFunctionBindingImpl : public MemberFunctionBinding
	{
	public:
		using fn_type = FN;
		MemberFunctionBindingImpl(fn_type fn) : m_fn{ fn } {}

		BoxedValue do_call(runtime::DispatchEngine & en,
						   BoxedValue & inst,
						   std::vector<BoxedValue> & args) const
		{
			return meta::static_if(std::is_same <R, void>{})
				// function returns void
				.then([this](auto & en, auto & inst, auto & args)
			{
				do_call_impl(en, boxed_cast<T>(inst), args, std::index_sequence_for<Args ...>{});
				return BoxedValue{};
			})
				// function does not return void
				.else_([this](auto & en, auto & inst, auto & args)
			{
				return BoxedValue{
					do_call_impl(en, boxed_cast<T>(inst), args, std::index_sequence_for<Args ...>{})
				};
			})(en, inst, args);
		}

		const TypeInfo & get_class_type_info() const
		{
			return ::get_type_info<T>();
		}

	private:
		impl::FunctionCallMatchScore get_call_score(runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args) const override
		{
			return impl::get_function_call_score<Args...>(en, args, std::index_sequence_for<Args...>{});
		}
		template <std::size_t ... Is>
		impl::ResolveReturnType<R> do_call_impl(runtime::DispatchEngine & en,
												T & inst, std::vector<BoxedValue> & args,
												std::index_sequence<Is ...>) const
		{
			(void)en;
			(void)args;
			return (inst.*m_fn)(impl::ResolveParamType<Args>(args[Is], en) ...);
		}

		fn_type m_fn{ nullptr };
	};

	template <typename T, typename R, typename ... Args>
	using MemberFnBinding = MemberFunctionBindingImpl<T, R(T::*)(Args...), R, Args...>;

	template <typename T, typename R, typename ... Args>
	using ConstMemberFnBinding = MemberFunctionBindingImpl<T, R(T::*)(Args...) const, R, Args...>;

}

// variable bindings
namespace binds
{
	class GlobalVariableBinding
	{
	public:
		virtual ~GlobalVariableBinding() = default;
		GlobalVariableBinding(BoxedValue && binded_var) : m_v{ std::move(binded_var) } {};
		GlobalVariableBinding(GlobalVariableBinding &&) = default;
		GlobalVariableBinding& operator=(GlobalVariableBinding &&) = default;

		BoxedValue & get_variable() { return m_v; }

	private:
		BoxedValue m_v;
	};

	class MemberVariableBinding
	{
	public:
		virtual BoxedValue get_variable(BoxedValue & instance) const = 0;
		virtual const TypeInfo & get_class_type_info() const = 0;
	};

	template <typename CLASS, typename T>
	class MemVarBinding : public MemberVariableBinding
	{
	public:
		using variable_type = T CLASS::*;
	public:
		explicit MemVarBinding(variable_type mem_var)
			: m_member_var{ mem_var }
		{}

		BoxedValue get_variable(BoxedValue & instance) const override
		{
			CLASS & class_inst = boxed_cast<CLASS>(instance);
			return{ BoxedValueStoreRef_t{}, class_inst.*m_member_var };
		}
		const TypeInfo & get_class_type_info() const override
		{
			return ::get_type_info<CLASS>();
		}

	private:
		variable_type m_member_var{ nullptr };

	};
}

// type conversions
namespace binds
{
	class ITypeConversion
	{
	public:
		template <typename T>
		T convert(const BoxedValue & bv) const
		{
			return convert(bv).get_as<T>();
		}

		virtual BoxedValue convert(const BoxedValue & bv) const = 0;
		virtual type_pair_key get_type_pair_hash() const = 0;
	};

	template <typename FROM, typename TO>
	class TypeConversion : public ITypeConversion
	{
	public:
		BoxedValue convert(const BoxedValue & bv) const override
		{
			return BoxedValue{ static_cast<TO>(boxed_cast<FROM>(bv)) };
		}
		type_pair_key get_type_pair_hash() const override
		{
			return ::get_type_pair_hash<FROM, TO>();
		}
	};

	// constructor
	template <typename T, typename ... Ts>
	struct TypeConstructor
	{
		static T construct(Ts && ... vs)
		{
			return T(std::forward<Ts>(vs) ...);
		}
	};

	namespace impl
	{
		template <typename T, typename ... Args>
		auto get_type_ctor(T(*)(Args ...))
		{
			return TypeConstructor<T, Args...>::construct;
		}
	}
}

#include <functional>

// helpers to create bindings
namespace binds
{
	struct non_const_t {};
	struct const_t {};

	// std::function
	template <typename R, typename ... Ts>
	auto func(std::function<R(Ts ...)> && fn)
	{
		return std::make_unique<GlobalFnBinding<R, Ts ...>>(std::move(fn));
	}
	
	// global function
	template <typename R, typename ... Ts>
	auto func(R(*fn)(Ts ...))
	{
		return func(std::function<R(Ts ...)>{ fn });
	}

	// lambda function
	template <typename F, typename T>
	auto func(T && fn)
	{
		return func(std::function<F>(std::forward<T>(fn)));
	}

	namespace impl
	{
		template <typename T, typename F>
		using member_func_T_resolver = std::conditional_t<
			std::is_base_of<T, std::function<F>>::value,
			std::function<F>,
			T
		>;
	}

	// member function
	template <typename T, typename R, typename ... Args>
	auto func(R(T::* fn)(Args ...), non_const_t = non_const_t())
	{
		using real_t = impl::member_func_T_resolver<T, R(Args...)>;
		return std::make_unique<MemberFnBinding<real_t, R, Args ...>>(fn);
	}

	// const member function
	template <typename T, typename R, typename ... Args>
	auto func(R(T::* fn)(Args ...) const, const_t = const_t())
	{
		using real_t = impl::member_func_T_resolver<T, R(Args...)>;
		return std::make_unique<ConstMemberFnBinding<real_t, R, Args ...>>(fn);
	}

	// global variables
	template <typename T>
	GlobalVariableBinding var(T & x)
	{
		// we want to sice this
		return GlobalVariableBinding{ BoxedValue{ BoxedValueStoreRef_t{}, x } };
	}

	// member variables
	template <typename CLASS, typename T>
	std::unique_ptr<MemberVariableBinding> var(T CLASS::* mem_var)
	{
		// we want to sice this
		return std::make_unique<MemVarBinding<CLASS, T>>(mem_var);
	}

	// conversion operator
	template <typename FROM, typename TO>
	std::unique_ptr<ITypeConversion> conv()
	{
		return std::make_unique<TypeConversion<FROM, TO>>();
	}

	// constructor
	template <typename F>
	auto ctor()
	{
		return func<F>(impl::get_type_ctor(std::add_pointer_t<F>{ nullptr }));
	}
}


