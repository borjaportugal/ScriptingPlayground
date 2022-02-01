
#include "DispatchEngine.h"
#include "AST.h"
#include "Bindings.h"

#include "RuntimeException.h"

#include <iostream>

namespace binds
{
#pragma region // operators
	template <typename T1, typename T2>
	void add_common_operations(BinaryOperators & group)
	{
		using namespace opts;
		group.add_operators<T1, T2,
			Add, Sub, Mul, Div, Eq,
			AddEq, SubEq, MulEq, DivEq>();
	}
	template <typename T1, typename T2>
	void add_bitwise_operations(BinaryOperators & group)
	{
		using namespace opts;
		group.add_operators<T1, T2,
			And, Or, Xor, LeftShift, RightShift,
			AndEq, OrEq, XorEq, LeftShiftEq, RightShiftEq>();
	}
	template <typename T1, typename T2>
	void add_comparison_operations(BinaryOperators & group)
	{
		using namespace opts;
		group.add_operators<T1, T2,
			EqEq, NotEq,
			Less, Greater,
			LessEq, GreaterEq>();
	}
	template <typename T1, typename T2>
	void add_logical_operations(BinaryOperators & group)
	{
		using namespace opts;
		group.add_operators<T1, T2,
			LogicOr, LogicAnd>();
	}

	template <typename T1, typename T2>
	void add_comparison_and_logical_operations(BinaryOperators & group)
	{
		add_logical_operations<T1, T2>(group);
		add_comparison_operations<T1, T2>(group);
	}

	template <typename T1, typename T2>
	void add_integer_operations(BinaryOperators & group)
	{
		using namespace opts;
		add_common_operations<T1, T2>(group);
		add_bitwise_operations<T1, T2>(group);
		group.add_operators<T1, T2, Mod, ModEq>();
	}

	void add_default_binary_operations(BinaryOperators & group)
	{
		add_common_operations<int, float>(group);
		add_common_operations<int, double>(group);
		add_common_operations<float, float>(group);
		add_common_operations<float, double>(group);
		add_common_operations<double, double>(group);

		add_integer_operations<int, int>(group);
		add_integer_operations<unsigned int, int>(group);
		add_integer_operations<unsigned int, unsigned int>(group);
		add_integer_operations<std::size_t, unsigned int>(group);
		add_integer_operations<std::size_t, int>(group);
		add_integer_operations<std::size_t, std::size_t>(group);
		add_integer_operations<char, char>(group);
		add_integer_operations<char, int>(group);
		add_integer_operations<char, unsigned int>(group);
		add_integer_operations<char, std::size_t>(group);

		add_comparison_operations<int, std::size_t>(group);
		add_comparison_operations<int, float>(group);
		add_comparison_operations<std::size_t, float>(group);

		add_comparison_and_logical_operations<char, char>(group);
		add_comparison_and_logical_operations<int, int>(group);
		add_comparison_and_logical_operations<unsigned int, unsigned int>(group);
		add_comparison_and_logical_operations<float, float>(group);
		add_comparison_and_logical_operations<float, double>(group);
		add_comparison_and_logical_operations<double, double>(group);

		using namespace opts;
		add_logical_operations<bool, bool>(group);
		group.add_operators<bool, bool, Eq, EqEq, NotEq>();
	}
#pragma endregion

#pragma region // conversion
	namespace impl
	{
		template <typename T, typename ... Ts>
		struct add_conversions_impl
		{
			static void add(runtime::DispatchEngine & eng)
			{
				std::initializer_list<int>{ (eng.add(binds::conv<T, Ts>()), 0) ... };
				std::initializer_list<int>{ (eng.add(binds::conv<Ts, T>()), 0) ... };
				add_conversions_impl<Ts ...>::add(eng);
			}
		};
		template <typename T>
		struct add_conversions_impl<T>
		{
			static void add(runtime::DispatchEngine &) {}
		};
	}

	template <typename ... Ts>
	void add_conversions_for_types(runtime::DispatchEngine & eng)
	{
		impl::add_conversions_impl<Ts...>::add(eng);
	}

	void add_default_conversions(runtime::DispatchEngine & eng)
	{
		add_conversions_for_types<char, int, unsigned int, std::size_t>(eng);
		add_conversions_for_types<float, double>(eng);
	}
#pragma endregion

	template <typename T>
	void add_vector_functions(runtime::DispatchEngine & eng)
	{
		using value_type = typename T::value_type;
		using iterator = typename T::iterator;
		using const_iterator = typename T::const_iterator;

		eng.add("size", func(&T::size));
		eng.add("push_back", func<T, void, value_type &&>(&T::push_back));
		eng.add("pop_back", func(&T::pop_back));
		eng.add("empty", func(&T::empty));
		eng.add("resize", func<T, void, std::size_t>(&T::resize));
		eng.add("reserve", func(&T::reserve));
		eng.add("capacity", func(&T::capacity));
		eng.add("begin", func(&T::begin, binds::non_const_t{}));

		eng.add("[]", func(&T::operator[], binds::non_const_t{}));
	}

	template <typename std_string_type>
	void add_string_functions(runtime::DispatchEngine & eng, BinaryOperators & binary_operators)
	{
		eng.add("size", func(&std_string_type::size));
		eng.add("length", func(&std_string_type::length));
		eng.add("push_back", func(&std_string_type::push_back));
		eng.add("substr", func(&std_string_type::push_back));
		eng.add("[]", func(&std_string_type::operator[], binds::non_const_t{}));

		using namespace opts;
		binary_operators.add_operators<std_string_type, std_string_type,
			// modification
			Eq, Add, AddEq,
			// comparison
			EqEq, NotEq,
			Less, Greater,
			LessEq, GreaterEq>();
	}

#pragma region // print
	template <typename T>
	void add_printing_function_for(runtime::DispatchEngine & eng)
	{
		eng.add("print", func<void(const T &)>(
			[](const T & v)
		{
			std::cout << v;
		}));
	}

	template <typename ... Ts>
	void add_printing_functions_for(runtime::DispatchEngine & eng)
	{
		std::initializer_list<int>{ (add_printing_function_for<Ts>(eng), 0) ... };
	}
#pragma endregion

	void add_default_printing_functions(runtime::DispatchEngine & eng)
	{
		add_printing_functions_for<char, int, float, std::size_t, std::string>(eng);
	}

	void add_default_members(runtime::DispatchEngine & eng)
	{
		add_vector_functions<std::vector<BoxedValue>>(eng);
	}

	namespace impl
	{
		template <typename FROM, typename TO>
		auto make_conversion_func()
		{
			return func<BoxedValue(FROM &)>(
				[](FROM & val) -> BoxedValue
			{
				return BoxedValue{ static_cast<TO>(val) };
			});
		}
	}

	template <typename FROM, typename ... TOs>
	void add_explicit_conversions(runtime::DispatchEngine & eng, const char * name)
	{
		std::initializer_list<int>{ 
			(eng.add(name, impl::make_conversion_func<FROM, TOs>()), 0) ... 
		};
	}

	void add_default_construction_functions(runtime::DispatchEngine & eng)
	{
		add_explicit_conversions<float, int>(eng, "int");
		add_explicit_conversions<int, float>(eng, "float");
	}

	void add_all_default(runtime::DispatchEngine & eng, BinaryOperators & binary_opts)
	{
		add_default_binary_operations(binary_opts);
		add_default_members(eng);
		add_default_conversions(eng);
		add_string_functions<std::string>(eng, binary_opts);
		//add_default_printing_functions(eng);
		add_default_construction_functions(eng);

		eng.add("assert", func<void(bool)>(
			[](bool b)
		{
			if (!b)
				throw except::AssertionFailure{ "Assertion failed!" };
		}));
		eng.add("assert", func<void(bool, const char *)>(
			[](bool b, const char * what)
		{
			if (!b)
				throw except::AssertionFailure{ what };
		}));
	}
}

namespace binds
{
	void ClassBindings::add(std::string name, std::unique_ptr<MemberFunctionBinding> && fn)
	{
		auto it = m_member_functions.find(name);

		// first function with this name
		if (it == m_member_functions.end())
			m_member_functions.emplace(std::move(name), std::move(fn));
		else
		{
			// overloaded function
			auto & old_function = it->second;

			// include it in its overloads
			if (auto * overloaded = dynamic_cast<binds::OverloadedMemberFunctionBinding *>(old_function.get()))
				overloaded->add_overload(std::move(fn));
			else
			{
				// create an overloaded function type and store both overloads
				std::unique_ptr<binds::MemberFunctionBinding> func1{ dynamic_cast<binds::MemberFunctionBinding *>(old_function.release()) };
				old_function = std::make_unique<binds::OverloadedMemberFunctionBinding>(std::move(func1), std::move(fn));
			}
		}
	}
	void ClassBindings::add(std::string name, std::unique_ptr<MemberVariableBinding> && var)
	{
		const auto it = m_member_varaibles.emplace(std::move(name), std::move(var));
		if (!it.second)
		{
			SCR_RUNTIME_EXCEPTION("Already exists member variable on type '",
								  it.first->second->get_class_type_info().get_bare_std_type_info().name(),
								  "' named '", name, "'.");
		}
	}

	const IMemberFunctionBinding * ClassBindings::get_member_func(const std::string & name) const
	{
		const auto it = m_member_functions.find(name);
		return it != m_member_functions.end() ? it->second.get() : nullptr;
	}
	const MemberVariableBinding * ClassBindings::get_member_var(const std::string & name) const
	{
		const auto it = m_member_varaibles.find(name);
		return it != m_member_varaibles.end() ? it->second.get() : nullptr;
	}
}

namespace runtime
{
	DispatchEngine::StackScopeGuard::StackScopeGuard(Stack & stk)
		: m_stk(stk)
	{
		m_stk.push_new_scope();
	}
	DispatchEngine::StackScopeGuard::~StackScopeGuard()
	{
		m_stk.pop_scope();
	}

	DispatchEngine::DispatchEngine()
	{
		// TODO(Borja): we should be able to add operatos without exposing m_binary_operators
		binds::add_all_default(*this, m_binary_opts);
	}

	void DispatchEngine::add(std::string name, std::unique_ptr<binds::GlobalFunctionBinding> && fn)
	{
		auto it = m_global_functions.find(name);

		// first function with this name
		if (it == m_global_functions.end())
			m_global_functions.emplace(std::move(name), std::move(fn));
		else
		{
			// is an overloaded function
			auto & old_function = it->second;

			// include it in its overloads
			if (auto * overloaded = dynamic_cast<binds::OverloadedGlobalFunctionBinding *>(old_function.get()))
				overloaded->add_overload(std::move(fn));
			else
			{
				// create an overloaded function type and store both overloads
				std::unique_ptr<binds::GlobalFunctionBinding> func1{ dynamic_cast<binds::GlobalFunctionBinding *>(old_function.release()) };
				old_function = std::make_unique<binds::OverloadedGlobalFunctionBinding>(std::move(func1), std::move(fn));
			}
		}
	}
	void DispatchEngine::add(std::string name, binds::GlobalVariableBinding && var)
	{
		m_global_scope.create_variable(std::move(name), std::move(var.get_variable()));
	}
	void DispatchEngine::add(std::string name, std::unique_ptr<binds::MemberFunctionBinding> && fn)
	{
		const std::type_info & class_type = fn->get_class_type_info().get_std_type_info();
		m_type_bindings[class_type].add(std::move(name), std::move(fn));
	}
	void DispatchEngine::add(std::string name, std::unique_ptr<binds::MemberVariableBinding> && member_var)
	{
		const std::type_info & class_type = member_var->get_class_type_info().get_std_type_info();
		m_type_bindings[class_type].add(std::move(name), std::move(member_var));
	}
	void DispatchEngine::add(std::unique_ptr<binds::ITypeConversion> && type_conv)
	{
		const auto key = type_conv->get_type_pair_hash();
		m_type_conversions[key] = std::move(type_conv);
	}
	
	const binds::IGlobalFunctionBinding * DispatchEngine::get_global_fn(const std::string & fn_name) const
	{
		const auto it = m_global_functions.find(fn_name);
			return it != m_global_functions.end() ? it->second.get() : nullptr;
	}
	const binds::ClassBindings * DispatchEngine::get_class_bindings(const TypeInfo & type) const
	{
		const auto it = m_type_bindings.find(type.get_bare_std_type_info());
		return it != m_type_bindings.end() ? &it->second : nullptr;
	}

	const binds::ITypeConversion * DispatchEngine::get_type_conversion(const TypeInfo & from, 
		const TypeInfo & to) const
	{
		const auto key = get_type_pair_hash(from, to);
		const auto it = m_type_conversions.find(key);
		return it != m_type_conversions.end() ? it->second.get() : nullptr;
	}

	BoxedValue DispatchEngine::evaluate(ast::ASTNode & root)
	{
		m_stack.clear_all();
		return root.evaluate(*this);
	}

	DispatchEngine::StackScopeGuard DispatchEngine::new_scope()
	{
		return{ m_stack };
	}

	BoxedValue & DispatchEngine::create_variable(const std::string & name, BoxedValue && bv)
	{
		return m_stack.create_variable(name, std::move(bv));
	}

	const binds::BinaryOperators::operation_fn *
		DispatchEngine::get_binary_operator(const TypeInfo & lhs,
			OperatorType op,
			const TypeInfo & rhs) const
	{
		return m_binary_opts.get_operator(lhs, op, rhs);
	}

	BoxedValue * DispatchEngine::get_variable(const std::string & name)
	{
		// variables in the stack (inside scopes) oclude global variables
		if (BoxedValue * p_val = m_stack.get_variable(name))
			return p_val;

		// not in the stack, check global scope
		if (BoxedValue * p_val = m_global_scope.get_variable(name))
			return p_val;

		return nullptr;
	}
	BoxedValue * DispatchEngine::get_stack_variable(const std::string & name)
	{
		return m_stack.get_variable(name);
	}
	BoxedValue * DispatchEngine::get_global_variable(const std::string & name)
	{
		return m_global_scope.get_variable(name);
	}

	std::size_t DispatchEngine::get_variable_num() const
	{
		return m_stack.get_var_num();
	}

}
