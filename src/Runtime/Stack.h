
#pragma once

#include "BoxedValue.h"

#include <string>	// std::string
#include <vector>	// std::vector
#include <map>		// std::map

namespace runtime
{
	struct Scope
	{
		BoxedValue * get_variable(const std::string & name);
		BoxedValue & create_variable(std::string name, BoxedValue && bv);

		std::size_t get_var_num() const;

	private:
		std::map<std::string, BoxedValue>	m_scope_vars;
	};

	class Stack
	{
	public:
		Stack();
		Stack(const Stack &) = delete;
		Stack& operator=(const Stack &) = delete;

		BoxedValue * get_variable(const std::string & name);
		BoxedValue & create_variable(const std::string & name, BoxedValue && bv = BoxedValue{});

		void clear_all();
		void push_new_scope();
		void pop_scope();

		/// \brief	Returns the number of local variables that are accesible from the
		///			current scope.
		std::size_t get_var_num() const;

	private:
		Scope & get_curr_stack_frame();
		const Scope & get_curr_stack_frame() const;

		std::vector<Scope> m_stack_scopes;
	};
}
