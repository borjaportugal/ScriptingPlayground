
#include "AST.h"

#include "Runtime\OperatorType.h"
#include "DispatchEngine.h"	// runtime::DispatchEngine
#include "RuntimeException.h"

#include "Parse\OperatorParsing.h"

#include "static_if.h"		// meta::static_if

#include <map>	// std::map

namespace ast
{
	Statements::Statements(std::vector<std::unique_ptr<ASTNode>> && statements)
		: m_statements{ std::move(statements) }
	{}
	BoxedValue Statements::evaluate(runtime::DispatchEngine & en) const
	{
		if (m_statements.empty())	return{};

		for (unsigned i = 0; i < m_statements.size() - 1; ++i)
			m_statements[i]->evaluate(en);

		return m_statements.back()->evaluate(en);
	}

	Scope::Scope(std::vector<std::unique_ptr<ASTNode>> && statements)
		: Statements(std::move(statements))
	{}
	BoxedValue Scope::evaluate(runtime::DispatchEngine & en) const
	{
		auto scope = en.new_scope();
		return Statements::evaluate(en);
	}

	BinaryOperator::BinaryOperator(OperatorType op)
		: m_operator(op)
	{}
	BoxedValue BinaryOperator::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue lhs = m_lhs->evaluate(en);
		BoxedValue rhs = m_rhs->evaluate(en);
		BoxedValue & real_lhs = resolve_ref(lhs);
		BoxedValue & real_rhs = resolve_ref(rhs);

		if (m_operator == OperatorType::EQ && real_lhs.empty())
		{
			// we need to change the value of the variable real_lhs is storing
			real_lhs = real_rhs;

			// operator= returns *this
			return lhs;
		}

		const auto * op = en.get_binary_operator(real_lhs.get_type_info(), m_operator,
												 real_rhs.get_type_info());
		if (op)		return (*op)(real_lhs, real_rhs);

		SCR_RUNTIME_EXCEPTION("Cannot perform operation: ", 
							  real_lhs.get_type_info().get_std_type_info().name(),
							  parse::get_operator_str(m_operator).c_str(),
							  real_rhs.get_type_info().get_std_type_info().name());
	}
	void BinaryOperator::set_operands(std::unique_ptr<ASTNode> && lhs,
									  std::unique_ptr<ASTNode> && rhs)
	{
		m_lhs = std::move(lhs);
		m_rhs = std::move(rhs);
	}

	OperatorType BinaryOperator::get_operator_type() const { return m_operator; }
	bool BinaryOperator::has_operands() const
	{
		// operands need to be ser in pairs, if one is nullptr, both will be
		return (m_lhs != nullptr);
	}

	namespace impl
	{
		template <typename T>
		BoxedValue perform_common_unary_operation(T & x, OperatorType op)
		{
			switch (op)
			{
				case OperatorType::POST_INC:
				case OperatorType::PRE_INC:
					return BoxedValue{ x + 1 };
				case OperatorType::PRE_DEC:
				case OperatorType::POST_DEC:
					return BoxedValue{ x - 1 };

				case OperatorType::UNARY_MINUS:	return BoxedValue{ -x };
				case OperatorType::LOGIC_NOT:	return BoxedValue{ !x };
			}

			SCR_RUNTIME_EXCEPTION("Invalid unary operator ", parse::get_operator_str(op));
		}

		template <typename T>
		BoxedValue perform_unary_operation(T & x, OperatorType op)
		{
			return meta::static_if(std::is_same<T, bool>{})
				.then([](auto & x, OperatorType op)
			{
				switch (op)
				{
					case OperatorType::LOGIC_NOT:	return BoxedValue{ !x };
				}

				SCR_RUNTIME_EXCEPTION("Invalid unary operator for type bool ", parse::get_operator_str(op));
			})
				.else_if(std::is_integral<T>{})
				.then([](auto & x, OperatorType op)
			{
				switch (op)
				{
					case OperatorType::BITWISE_NOT:	return BoxedValue{ ~x };
				}

				return perform_common_unary_operation(x, op);
			})
				.else_([](auto & x, OperatorType op)
			{
				return perform_common_unary_operation(x, op);
			})(x, op);
		}

		BoxedValue perform_unary_operation(BoxedValue & bv, OperatorType op)
		{
			const auto & typeinfo = bv.get_type_info();

			if (typeinfo == get_type_info<int>())
				return perform_unary_operation(boxed_cast<int>(bv), op);
			else if (typeinfo == get_type_info<float>())
				return perform_unary_operation(boxed_cast<float>(bv), op);
			else if (typeinfo == get_type_info<bool>())
				return perform_unary_operation(boxed_cast<bool>(bv), op);

			SCR_RUNTIME_EXCEPTION("Invalid unary operator ", parse::get_operator_str(op));
		}
	}

	UnaryOperator::UnaryOperator(OperatorType op, std::unique_ptr<ASTNode> && var)
		: m_operator(op)
		, m_variable(std::move(var))
	{}
	BoxedValue UnaryOperator::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue bv = m_variable->evaluate(en);
		BoxedValue & real_val = resolve_ref(bv);

		if (m_operator == OperatorType::UNARY_PLUS)	return real_val;

		// STUDY(Borja): in here we could create less boxed values
		// in the pre-increment and pre-decrement we could change directly the variable stored in the boxed value
		// and avoid creating the boxed value returned by impl::preform_unary_operation
		if (m_operator == OperatorType::PRE_DEC || m_operator == OperatorType::PRE_INC)
		{
			real_val = impl::perform_unary_operation(real_val, m_operator);
			return real_val;
		}
		else if (m_operator == OperatorType::POST_DEC || m_operator == OperatorType::POST_INC)
		{
			auto temp = real_val;
			real_val = impl::perform_unary_operation(real_val, m_operator);
			return temp;
		}

		return impl::perform_unary_operation(real_val, m_operator);
	}

	BoxedValue Value::evaluate(runtime::DispatchEngine &) const
	{
		return BoxedValue{ m_value };
	}

	NamedVariable::NamedVariable(std::string && name, bool declaration)
		: m_variable_name(std::move(name))
		, m_declaration(declaration)
	{}
	BoxedValue NamedVariable::evaluate(runtime::DispatchEngine & en) const
	{
		if (m_declaration)
			return make_ref(en.create_variable(m_variable_name));

		if (auto * var = en.get_variable(m_variable_name))
			return make_ref(*var);

		SCR_RUNTIME_EXCEPTION("Trying to get an unused variable.");
	}

	namespace impl
	{
		bool evaluates_to_true(const BoxedValue & bv)
		{
			const auto & typeinfo = bv.get_type_info();
			if (typeinfo == get_type_info<bool>())			return boxed_cast<bool>(bv);
			else if (typeinfo == get_type_info<int>())		return boxed_cast<int>(bv) != 0;
			else if (typeinfo == get_type_info<float>())	return boxed_cast<float>(bv) != 0.f;

			SCR_RUNTIME_EXCEPTION("Value of type ", typeinfo.get_bare_std_type_info().name(), " cannot be evaluated to true or false.");
		}
	}

	If::If(std::unique_ptr<ASTNode> && cond,
		   std::unique_ptr<ASTNode> && statements,
		   std::unique_ptr<ASTNode> && else_)
		: m_condition(std::move(cond))
		, m_statements(std::move(statements))
		, m_else(std::move(else_))
	{}

	BoxedValue If::evaluate(runtime::DispatchEngine & en) const
	{
		const BoxedValue condition_result = m_condition->evaluate(en);
		if (impl::evaluates_to_true(resolve_ref(condition_result)))
			m_statements->evaluate(en);
		else if (m_else)
			m_else->evaluate(en);

		return{};
	}

	While::While(std::unique_ptr<ASTNode> && cond,
				 std::unique_ptr<ASTNode> && statements)
		: m_condition(std::move(cond))
		, m_statements(std::move(statements))
	{}

	BoxedValue While::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue cond = m_condition->evaluate(en);
		while (impl::evaluates_to_true(resolve_ref(cond)))
		{
			m_statements->evaluate(en);

			cond = m_condition->evaluate(en);
		}

		return{};
	}

	For::For(std::unique_ptr<ASTNode> && left,
			 std::unique_ptr<ASTNode> && mid,
			 std::unique_ptr<ASTNode> && right,
			 std::unique_ptr<ASTNode> && statements)
		: m_left(std::move(left))
		, m_condition(std::move(mid))
		, m_right(std::move(right))
		, m_statements(std::move(statements))
	{}
	BoxedValue For::evaluate(runtime::DispatchEngine & en) const
	{
		m_left->evaluate(en);

		BoxedValue cond = m_condition->evaluate(en);
		while (impl::evaluates_to_true(resolve_ref(cond)))
		{
			m_statements->evaluate(en);
			m_right->evaluate(en);
			cond = m_condition->evaluate(en);
		}

		return{};
	}

	namespace impl
	{
		StatementList::StatementList(std::vector<std::unique_ptr<ASTNode>> && statements)
			: m_statement_list(std::move(statements))
		{}

		std::vector<BoxedValue> StatementList::evaluate_all(runtime::DispatchEngine & en) const
		{
			std::vector<BoxedValue> results;
			results.reserve(m_statement_list.size());
			for (const auto & node : m_statement_list)
				results.emplace_back(node->evaluate(en));

			return results;
		}
		std::size_t StatementList::get_num() const
		{
			return m_statement_list.size();
		}
	}

	VectorDecl::VectorDecl(std::vector<std::unique_ptr<ASTNode>> && init_list)
		: m_init_list{ std::move(init_list) }
	{}
	BoxedValue VectorDecl::evaluate(runtime::DispatchEngine & en) const
	{
		return BoxedValue{ m_init_list.evaluate_all(en) };
	}

	namespace impl
	{
		BoxedValue perform_member_function_call(runtime::DispatchEngine & en, const std::string & fn_name,
												BoxedValue & inst, std::vector<BoxedValue> & params)
		{
			if (const auto * class_bindings = en.get_class_bindings(inst.get_type_info()))
			{
				if (const auto * member_fn = class_bindings->get_member_func(fn_name))
					return member_fn->do_call(en, inst, params);
				else
				{
					if (auto * maybe_callable_var = class_bindings->get_member_var(fn_name))
					{
						BoxedValue var = maybe_callable_var->get_variable(inst);
						std::cout << var.get_type_info().get_bare_std_type_info().name() << '\n';
						std::cout << get_type_info<std::function<int(int, int)>>().get_bare_std_type_info().name() << '\n';
						return impl::perform_member_function_call(en, "()", resolve_ref(var), params);
					}
					else
					{
						SCR_RUNTIME_EXCEPTION("Type ",
											  inst.get_type_info().get_bare_std_type_info().name(),
											  " does not have the function '", fn_name, "' bound.");
					}
				}
			}

			SCR_RUNTIME_EXCEPTION("No data for type ", inst.get_type_info().get_bare_std_type_info().name(), " found.");
		}
	}

	GlobalFunctionCall::GlobalFunctionCall(std::string && fn_name,
										   std::vector<std::unique_ptr<ASTNode>> && params)
		: m_fn_name{ std::move(fn_name) }
		, m_parameters{ std::move(params) }
	{}
	BoxedValue GlobalFunctionCall::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue result;
		if (const auto * fn = en.get_global_fn(m_fn_name))
		{
			auto args = m_parameters.evaluate_all(en);
			result = fn->do_call(en, args);

			if (except::is_boxed_error(result))
				SCR_RUNTIME_EXCEPTION("Not found a valid call to function '", m_fn_name, "'.");

			return std::move(result);
		}
		else if (BoxedValue * global_var = en.get_variable(m_fn_name))
		{
			if (const auto * class_binds = en.get_class_bindings(global_var->get_type_info()))
			{
				auto args = m_parameters.evaluate_all(en);
				return impl::perform_member_function_call(en, "()", *global_var, args);
			}
			else
			{
				SCR_RUNTIME_EXCEPTION("Trying to call function '", m_fn_name, "' found global variable of type '", global_var->get_type_info().get_bare_std_type_info().name(), "', but this type has not bound data.");
			}
		}

		SCR_RUNTIME_EXCEPTION("No function or callable object found with name '", m_fn_name, "'.");
	}
	
	MemberFunctionCall::MemberFunctionCall(std::string && fn_name,
										   std::unique_ptr<ASTNode> && inst,
										   std::vector<std::unique_ptr<ASTNode>> && params)
		: m_fn_name{ std::move(fn_name) }
		, m_instance{ std::move(inst) }
		, m_parameters{ std::move(params) }
	{}

	BoxedValue MemberFunctionCall::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue inst = m_instance->evaluate(en);
		BoxedValue & real_inst = resolve_ref(inst);
		auto params = m_parameters.evaluate_all(en);

		return impl::perform_member_function_call(en, m_fn_name, real_inst, params);
	}
	
	MemberVariableAccess::MemberVariableAccess(std::string && var_name,
												std::unique_ptr<ASTNode> && inst)
		: m_var_name{ std::move(var_name) }
		, m_instance{ std::move(inst) }
	{}

	BoxedValue MemberVariableAccess::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue inst = m_instance->evaluate(en);
		BoxedValue & real_inst = resolve_ref(inst);

		if (const auto * class_bind = en.get_class_bindings(real_inst.get_type_info()))
		{
			if (const auto * member_var = class_bind->get_member_var(m_var_name))
				return member_var->get_variable(real_inst);
			else
				SCR_RUNTIME_EXCEPTION("Type ", real_inst.get_type_info().get_bare_std_type_info().name(),
					" does not have the variable '", m_var_name, "' bound.");
		}

		SCR_RUNTIME_EXCEPTION("No data for type ", real_inst.get_type_info().get_bare_std_type_info().name(), " found.");
	}

	VectorAccess::VectorAccess(std::unique_ptr<ASTNode> && vec,
		std::unique_ptr<ASTNode> && index)
		: m_vector(std::move(vec))
		, m_index(std::move(index))
	{}
	BoxedValue VectorAccess::evaluate(runtime::DispatchEngine & en) const
	{
		BoxedValue inst = m_vector->evaluate(en);
		auto & real_inst = resolve_ref(inst);

		BoxedValue index_bv = m_index->evaluate(en);
		BoxedValue & index_bv_ref = resolve_ref(index_bv);

		// STUDY(Borja): checking if the value is an std::vector<BoxedValue> or std::string can improve performance
		// This way we don't have to search in the maps and perform more virtual calls...

		std::vector<BoxedValue> param{ index_bv_ref };
		return impl::perform_member_function_call(en, "[]", real_inst, param);
	}

}

namespace ast
{
	std::unique_ptr<Statements> make_statements(std::vector<std::unique_ptr<ASTNode>> && statements)
	{
		return std::make_unique<Statements>(std::move(statements));
	}
	std::unique_ptr<Scope> make_scope(std::vector<std::unique_ptr<ASTNode>> && statements)
	{
		return std::make_unique<Scope>(std::move(statements));
	}

	std::unique_ptr<BinaryOperator> make_operator(OperatorType op)
	{
		return std::make_unique<BinaryOperator>(op);
	}
	std::unique_ptr<UnaryOperator> make_unary_operator(OperatorType op,
													   std::unique_ptr<ASTNode> && variable)
	{
		return std::make_unique<UnaryOperator>(op, std::move(variable));
	}

	std::unique_ptr<ASTNode> make_named_variable(std::string && name, bool declaration)
	{
		return std::make_unique<NamedVariable>(std::move(name), declaration);
	}

	std::unique_ptr<If> make_if(std::unique_ptr<ASTNode> && cond,
								std::unique_ptr<ASTNode> && statements,
								std::unique_ptr<ASTNode> && else_)
	{
		return std::make_unique<If>(std::move(cond), std::move(statements), std::move(else_));
	}

	std::unique_ptr<While> make_while(std::unique_ptr<ASTNode> && cond,
									  std::unique_ptr<ASTNode> && statements)
	{
		return std::make_unique<While>(std::move(cond), std::move(statements));
	}

	std::unique_ptr<For> make_for(std::unique_ptr<ASTNode> && left,
								  std::unique_ptr<ASTNode> && mid,
								  std::unique_ptr<ASTNode> && right,
								  std::unique_ptr<ASTNode> && statements)
	{
		return std::make_unique<For>(std::move(left), std::move(mid), std::move(right),
									 std::move(statements));
	}

	std::unique_ptr<VectorDecl> make_vector_decl(std::vector<std::unique_ptr<ASTNode>> && init_list)
	{
		return std::make_unique<VectorDecl>(std::move(init_list));
	}
	std::unique_ptr<VectorAccess> make_vector_access(std::unique_ptr<ASTNode> && vec,
													 std::unique_ptr<ASTNode> && index)
	{
		return std::make_unique<VectorAccess>(std::move(vec), std::move(index));
	}

	std::unique_ptr<GlobalFunctionCall> make_global_fn_call(std::string && fn_name, std::vector<std::unique_ptr<ASTNode>> && params)
	{
		return std::make_unique<GlobalFunctionCall>(std::move(fn_name), std::move(params));
	}

	std::unique_ptr<Noop> make_noop()
	{
		return std::make_unique<Noop>();
	}

	std::unique_ptr<MemberFunctionCall> make_member_fn_call(
		std::string && fn_name,
		std::unique_ptr<ASTNode> && inst,
		std::vector<std::unique_ptr<ASTNode>> && params)
	{
		return std::make_unique<MemberFunctionCall>(std::move(fn_name), 
													std::move(inst), 
													std::move(params));
	}

	std::unique_ptr<MemberVariableAccess> make_member_var_access(std::string name, 
		std::unique_ptr<ASTNode> && inst)
	{
		return std::make_unique<ast::MemberVariableAccess>(std::move(name), std::move(inst));
	}

}

