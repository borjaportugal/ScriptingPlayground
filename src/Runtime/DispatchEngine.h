
#pragma once

#include "Forwards.h"
#include "Stack.h"
#include "Bindings.h"
#include "Runtime\Operators.h"

#include <typeindex>		// std::type_index
#include <unordered_map>	// std::unordered_map

namespace binds
{
	class ClassBindings
	{
	public:
		void add(std::string name, std::unique_ptr<MemberFunctionBinding> && fn);
		void add(std::string name, std::unique_ptr<MemberVariableBinding> && var);

		const IMemberFunctionBinding * get_member_func(const std::string & name) const;
		const MemberVariableBinding * get_member_var(const std::string & name) const;

	private:
		std::unordered_map<std::string, std::unique_ptr<IMemberFunctionBinding>> m_member_functions;
		std::unordered_map<std::string, std::unique_ptr<MemberVariableBinding>> m_member_varaibles;
	};
}

namespace runtime
{
	class DispatchEngine
	{
	private:
		class StackScopeGuard
		{
		public:
			StackScopeGuard(Stack & stk);
			~StackScopeGuard();
			StackScopeGuard(StackScopeGuard &&) = default;	// needed by DispatchEngine::new_scope
			StackScopeGuard(const StackScopeGuard &) = delete;
			StackScopeGuard& operator=(const StackScopeGuard &) = delete;

		private:
			Stack & m_stk;
		};

	public:
		DispatchEngine();

		///	\brief	Main function for evaluating an script
		BoxedValue evaluate(ast::ASTNode & root);

		void add(std::string name, std::unique_ptr<binds::GlobalFunctionBinding> && fn);
		void add(std::string name, binds::GlobalVariableBinding && var);
		void add(std::string name, std::unique_ptr<binds::MemberFunctionBinding> && fn);
		void add(std::string name, std::unique_ptr<binds::MemberVariableBinding> && member_var);
		void add(std::unique_ptr<binds::ITypeConversion> && type_conv);
		template <typename T1, typename T2, typename ... OPs>
		void add(binds::impl::OptBind<T1, T2, OPs ...>)
		{
			m_binary_opts.add_operators<T1, T2, OPs ...>();
		}

		const binds::ClassBindings * get_class_bindings(const TypeInfo & type) const;

		const binds::IGlobalFunctionBinding * get_global_fn(const std::string & fn_name) const;
		const binds::BinaryOperators::operation_fn * get_binary_operator(const TypeInfo & lhs,
																		 OperatorType op,
																		 const TypeInfo & rhs) const;
		const binds::ITypeConversion * get_type_conversion(const TypeInfo & from, const TypeInfo & to) const;
		
		BoxedValue * get_variable(const std::string & name);
		BoxedValue * get_stack_variable(const std::string & name);
		BoxedValue * get_global_variable(const std::string & name);

		template <typename T>
		T & get_variable_as(const std::string & name)
		{
			if (auto * p_var = get_variable(name))
				return boxed_cast<T>(*p_var);

			SCR_RUNTIME_EXCEPTION("Requeting unexistent variable with name '", name, "'");
		}

		template <typename T>
		T get_variable_value(const std::string & name)
		{
			BoxedValue var = *get_variable(name);
			if (!var.is_storing<T>())
			{
				if (auto * conv = get_type_conversion(var.get_type_info(), get_type_info<T>()))
					var = conv->convert(var);
			}

			return boxed_cast<T>(var);
		}

		std::size_t get_variable_num() const;

		StackScopeGuard new_scope();
		BoxedValue & create_variable(const std::string & name, BoxedValue && bv = BoxedValue{});

	private:
		Stack m_stack;
		Scope m_global_scope;

		std::unordered_map<std::string, std::unique_ptr<binds::IGlobalFunctionBinding>> m_global_functions;

		/// stores the member functions and variables of a class
		std::unordered_map<std::type_index, binds::ClassBindings> m_type_bindings;

		std::unordered_map<type_pair_key, std::unique_ptr<binds::ITypeConversion>> m_type_conversions;

		binds::BinaryOperators m_binary_opts;

	};
}

