
#pragma once

#include "Runtime\BoxedValue.h"
#include "Runtime\OperatorType.h"	// OperatorType
#include "static_if.h"				// meta::static_if

#include <typeinfo>
#include <unordered_map>

namespace binds
{
	class BinaryOperators
	{
	public:
		/// \brief	Signature of thefunction that will perform any binary operation
		using operation_fn = BoxedValue(*)(BoxedValue &, const BoxedValue&);
		
		/// \brief	Easy way to add multiple operators for types T1 and T2
		template <typename T1, typename T2, typename ... OPs>
		void add_operators()
		{
			static_assert(sizeof...(OPs) != 0, "Need to pass one operator at least.");
			std::initializer_list<int>{ (add_operator<T1, T2, OPs>(), 0) ... };
		}
		template <typename T1, typename T2, typename OP>
		void add_operator()
		{
			// if T1 and T2 are the same type we only need one operator
			// (i.e 'int+float' needs also 'float+int' but 'int+int' only needs that)
			meta::static_if(std::is_same<T1, T2>{})
				.then([this]
			{
				add_operator_impl<T1, T2, OP>();
			})
				.else_([this]
			{
				add_operator_impl<T1, T2, OP>();
				add_operator_impl<T2, T1, OP>();
			})();
		}

		const operation_fn * get_operator(const TypeInfo & lhs_type, OperatorType op,
										  const TypeInfo & rhs_type) const;

	private:
		template <typename T1, typename T2, typename OP>
		void add_operator_impl()
		{
			// lhs is non const in case the operator modifies it (i.e. += or -=)
			m_all_operations[OP::s_type][get_type_pair_hash<T1, T2>()] =
				[](BoxedValue & lhs, const BoxedValue & rhs)
			{
				return BoxedValue{ OP::call(boxed_cast<T1>(lhs), boxed_cast<T2>(rhs)) };
			};
		}

		///	\brief	Containter that will hold an operation for all the types.
		///			(i.e. for the operator '+' will hold float + float, float + int...
		using operator_operations = std::unordered_map<type_pair_key, operation_fn >;

		///	\brief	Holds all the operations for all the types
		std::unordered_map<OperatorType, operator_operations > m_all_operations;
	};

	namespace impl
	{
		template <typename T1, typename T2, typename ... OPs>
		struct OptBind {};
	}

	template <typename T1, typename T2, typename ... OPs>
	impl::OptBind<T1, T2, OPs ...> opts_for()
	{
		return{};
	}
}

#define DECLARE_OPERATOR(name, op, type)							\
		struct name {												\
			static constexpr OperatorType s_type = OperatorType::type;	\
			template <typename T1, typename T2>						\
			static auto call(const T1 & lhs, const T2 & rhs)		\
			{ return lhs op rhs; }							\
		}

#define DECLARE_COMPOUND_OPERATOR(name, op, type)					\
		struct name {												\
			static constexpr OperatorType s_type = OperatorType::type;	\
			template <typename T1, typename T2>						\
			static auto call(T1 & lhs, const T2 & rhs)				\
			{ return lhs op static_cast<T1>(rhs); }					\
		}

#define DECLARE_OPERATOR_AND_COMPOUND(name, op, type)		\
	DECLARE_OPERATOR(name, op, type);						\
	DECLARE_COMPOUND_OPERATOR(name##Eq, op=, type##_EQ)

namespace opts
{
	DECLARE_OPERATOR_AND_COMPOUND(Add, +, ADD);
	DECLARE_OPERATOR_AND_COMPOUND(Sub, -, SUB);
	DECLARE_OPERATOR_AND_COMPOUND(Mul, *, MUL);
	DECLARE_OPERATOR_AND_COMPOUND(Div, /, DIV);
	DECLARE_OPERATOR_AND_COMPOUND(Mod, %, MOD);

	DECLARE_OPERATOR_AND_COMPOUND(LeftShift, << , LEFT_SHIFT);
	DECLARE_OPERATOR_AND_COMPOUND(RightShift, >> , RIGHT_SHIFT);
	DECLARE_OPERATOR_AND_COMPOUND(And, &, AND);
	DECLARE_OPERATOR_AND_COMPOUND(Or, |, OR);
	DECLARE_OPERATOR_AND_COMPOUND(Xor, ^, XOR);

	DECLARE_OPERATOR_AND_COMPOUND(Less, <, LESS);
	DECLARE_OPERATOR_AND_COMPOUND(Greater, >, GREATER);
	DECLARE_OPERATOR(EqEq, ==, EQEQ);
	DECLARE_OPERATOR(NotEq, != , NOT_EQ);

	DECLARE_OPERATOR(LogicOr, ||, LOGIC_OR);
	DECLARE_OPERATOR(LogicAnd, &&, LOGIC_AND);
	
	// not really a compound operator but implementation works
	DECLARE_COMPOUND_OPERATOR(Eq, =, EQ);
}

#undef DECLARE_BIN_OPERATOR_AND_COMPUND
#undef DECLARE_COMPOUND_OPERATOR
#undef DECLARE_OPERATOR

// TODO(Borja): we need a better way to implement this
#define IMPLEMENT_CALL_FOR_OPERATOR(name, T1, name1, T2, name2, code1, code2)			\
	namespace opts							\
	{	\
		template <>						\
		inline auto name::call(const T1 & name1, const T2 & name2)	\
		{ code1 }	\
		template <>						\
		inline auto name::call(const T2 & name2, const T1 & name1)	\
		{ code2 }	\
	}

IMPLEMENT_CALL_FOR_OPERATOR(EqEq, int, signed_, std::size_t, unsigned_,
if (signed_ < 0)	return false;
return static_cast<std::size_t>(signed_) == unsigned_;
,
if (signed_ < 0)	return false;
return static_cast<std::size_t>(signed_) == unsigned_;
)

IMPLEMENT_CALL_FOR_OPERATOR(NotEq, int, signed_, std::size_t, unsigned_,
return !EqEq::call(signed_, unsigned_);
,
return !EqEq::call(unsigned_, signed_);
)

IMPLEMENT_CALL_FOR_OPERATOR(Less, int, signed_, std::size_t, unsigned_,
if (signed_ < 0) return true;
return static_cast<std::size_t>(signed_) < unsigned_;
, 
if (signed_ < 0) return false;
return unsigned_ < static_cast<std::size_t>(signed_);
)

IMPLEMENT_CALL_FOR_OPERATOR(Greater, int, signed_, std::size_t, unsigned_,
if (signed_ < 0) return true;
return static_cast<std::size_t>(signed_) < unsigned_;
,
if (signed_ < 0) return false;
return unsigned_ < static_cast<std::size_t>(signed_);
)

IMPLEMENT_CALL_FOR_OPERATOR(LessEq, int, signed_, std::size_t, unsigned_,
return EqEq::call(signed_, unsigned_) || Less::call(signed_, unsigned_);
,
return EqEq::call(unsigned_, signed_) || Less::call(unsigned_, signed_);
)

IMPLEMENT_CALL_FOR_OPERATOR(GreaterEq, int, signed_, std::size_t, unsigned_,
return EqEq::call(signed_, unsigned_) || Greater::call(signed_, unsigned_);
,
return EqEq::call(unsigned_, signed_) || Greater::call(unsigned_, signed_);
)


