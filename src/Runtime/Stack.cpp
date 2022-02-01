
#include "Stack.h"

#include "RuntimeException.h"

namespace runtime
{
	BoxedValue * Scope::get_variable(const std::string & name)
	{
		const auto it = m_scope_vars.find(name);
		return it != m_scope_vars.end() ? &it->second : nullptr;
	}
	BoxedValue & Scope::create_variable(std::string name, BoxedValue && bv)
	{
		const auto result = m_scope_vars.emplace(std::move(name), std::move(bv));
		if (result.second)
			return result.first->second;

		SCR_RUNTIME_EXCEPTION("Already exists a variable named '", name, "'");
	}
	std::size_t Scope::get_var_num() const
	{
		return m_scope_vars.size();
	}

	Stack::Stack()
		: m_stack_scopes(1)	// initialize the first stack frame (global)
	{}
	BoxedValue * Stack::get_variable(const std::string & name)
	{
		for (auto stk_it = m_stack_scopes.rbegin();
			 stk_it != m_stack_scopes.rend();
			 ++stk_it)
		{
			if (auto bv = stk_it->get_variable(name))
				return bv;
		}

		return nullptr;
	}

	Scope & Stack::get_curr_stack_frame()
	{
		return m_stack_scopes.back();
	}
	const Scope & Stack::get_curr_stack_frame() const
	{
		return m_stack_scopes.back();
	}

	void Stack::clear_all()
	{
		m_stack_scopes.clear();
		m_stack_scopes.emplace_back();
	}
	void Stack::push_new_scope()
	{
		m_stack_scopes.emplace_back();
	}
	void Stack::pop_scope()
	{
		m_stack_scopes.pop_back();
	}

	BoxedValue & Stack::create_variable(const std::string & name, BoxedValue && bv)
	{
		return get_curr_stack_frame().create_variable(name, std::move(bv));
	}

	std::size_t Stack::get_var_num() const
	{
		return get_curr_stack_frame().get_var_num();
	}
}
