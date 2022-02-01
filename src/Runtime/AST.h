#pragma once

#include "Forwards.h"	// runtime::DispatchEngine &
#include "BoxedValue.h"
#include "Runtime\OperatorType.h"

#include <memory>	// std::unique_ptr
#include <vector>	// std::vector

namespace ast
{
	/// \brief	Abstract Sintax Tree node to represent the tree containing operations
	class ASTNode
	{
	public:
		ASTNode() = default;
		virtual ~ASTNode() = default;
		ASTNode(ASTNode &&) = default;
		ASTNode& operator=(ASTNode &&) = default;
		ASTNode(const ASTNode &) = delete;
		ASTNode& operator=(const ASTNode &) = delete;

		virtual BoxedValue evaluate(runtime::DispatchEngine &) const = 0;
	};

	class Noop final : public ASTNode
	{
		BoxedValue evaluate(runtime::DispatchEngine &) const override { return{}; }
	};

	class Statements : public ASTNode
	{
	public:
		Statements() = default;
		Statements(Statements && other) = default;
		Statements& operator=(Statements && rhs) = default;
		explicit Statements(std::vector<std::unique_ptr<ASTNode>> && statements);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::vector<std::unique_ptr<ASTNode>> m_statements;
	};

	class Scope final : public Statements
	{
	public:
		explicit Scope(std::vector<std::unique_ptr<ASTNode>> && statements);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:

	};

	class BinaryOperator final : public ASTNode
	{
	public:
		explicit BinaryOperator(OperatorType op);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

		void set_operands(std::unique_ptr<ASTNode> && lhs,
						  std::unique_ptr<ASTNode> && rhs);

		OperatorType get_operator_type() const;
		bool has_operands() const;

	private:
		OperatorType m_operator;
		std::unique_ptr<ASTNode> m_lhs;
		std::unique_ptr<ASTNode> m_rhs;

	};
	class UnaryOperator final : public ASTNode
	{
	public:
		UnaryOperator(OperatorType op, std::unique_ptr<ASTNode> && var);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

		inline OperatorType get_operator_type() const { return m_operator; }

	private:
		OperatorType m_operator;
		std::unique_ptr<ASTNode> m_variable;

	};

	class Value final : public ASTNode
	{
	public:
		explicit Value(BoxedValue&& bv) : m_value(std::move(bv)) {}

		BoxedValue evaluate(runtime::DispatchEngine &) const override;

	private:
		BoxedValue m_value;

	};

	class NamedVariable final : public ASTNode
	{
	public:
		explicit NamedVariable(std::string && name, bool declaration = false);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		bool m_declaration{ false };	///< Determines if the variable needs to be created
		std::string m_variable_name;
	};

	class If final : public ASTNode
	{
	public:
		/// \note The parameter 'statement' can either be one unique statement or an Scope
		///	that contains all the statements of the scope followed by the if.
		If(std::unique_ptr<ASTNode> && cond,
		   std::unique_ptr<ASTNode> && statements,
		   std::unique_ptr<ASTNode> && else_);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::unique_ptr<ASTNode> m_condition;
		std::unique_ptr<ASTNode> m_statements;
		std::unique_ptr<ASTNode> m_else;
	};

	class While final : public ASTNode
	{
	public:
		While(std::unique_ptr<ASTNode> && cond,
			  std::unique_ptr<ASTNode> && statements);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::unique_ptr<ASTNode> m_condition;
		std::unique_ptr<ASTNode> m_statements;
	};

	class For final : public ASTNode
	{
	public:
		For(std::unique_ptr<ASTNode> && left,
			std::unique_ptr<ASTNode> && mid,
			std::unique_ptr<ASTNode> && right,
			std::unique_ptr<ASTNode> && statements);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::unique_ptr<ASTNode> m_left;
		std::unique_ptr<ASTNode> m_condition;
		std::unique_ptr<ASTNode> m_right;
		std::unique_ptr<ASTNode> m_statements;
	};

	namespace impl
	{
		class StatementList
		{
		public:
			explicit StatementList(std::vector<std::unique_ptr<ASTNode>> && statements);

			std::vector<BoxedValue> evaluate_all(runtime::DispatchEngine & en) const;
			std::size_t get_num() const;

		private:
			std::vector<std::unique_ptr<ASTNode>> m_statement_list;
		};
	}

	class VectorDecl final : public ASTNode
	{
	public:
		VectorDecl(std::vector<std::unique_ptr<ASTNode>> && init_list);
		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		impl::StatementList m_init_list;
	};

	class GlobalFunctionCall final : public ASTNode
	{
	public:
		GlobalFunctionCall(std::string && fn_name,
						   std::vector<std::unique_ptr<ASTNode>> && params);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::string m_fn_name;
		impl::StatementList m_parameters;
	};
	class MemberFunctionCall final : public ASTNode
	{
	public:
		MemberFunctionCall(std::string && fn_name,
						   std::unique_ptr<ASTNode> && inst,
						   std::vector<std::unique_ptr<ASTNode>> && params);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::string m_fn_name;
		std::unique_ptr<ASTNode> m_instance;
		impl::StatementList m_parameters;
	};

	class MemberVariableAccess final : public ASTNode
	{
	public:
		MemberVariableAccess(std::string && var_name, 
							std::unique_ptr<ASTNode> && inst);

		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::string m_var_name;
		std::unique_ptr<ASTNode> m_instance;

	};

	class VectorAccess final : public ASTNode
	{
	public:
		VectorAccess(std::unique_ptr<ASTNode> && vec,
			std::unique_ptr<ASTNode> && index);
		BoxedValue evaluate(runtime::DispatchEngine & en) const override;

	private:
		std::unique_ptr<ASTNode> m_vector;
		std::unique_ptr<ASTNode> m_index;
	};

}

// functions to create ast nodes less verbosely
namespace ast
{
	std::unique_ptr<Noop> make_noop();

	std::unique_ptr<Statements> make_statements(std::vector<std::unique_ptr<ASTNode>> && statements);
	std::unique_ptr<Scope> make_scope(std::vector<std::unique_ptr<ASTNode>> && statements);

	std::unique_ptr<BinaryOperator> make_operator(OperatorType op);
	std::unique_ptr<UnaryOperator> make_unary_operator(OperatorType op,
													   std::unique_ptr<ASTNode> && variable);

	template <typename T1, typename T2>
	std::unique_ptr<BinaryOperator> make_operator(T1 v1, OperatorType op, T2 v2)
	{
		auto new_op = make_operator(op);
		new_op->set_operands(
			make_value(v1),
			make_value(v2)
		);
		return new_op;
	}

	template <typename T>
	std::unique_ptr<Value> make_value(T v)
	{
		return std::make_unique<Value>(BoxedValue{ v });
	}

	std::unique_ptr<ASTNode> make_named_variable(std::string && name,
												 bool declaration = false);

	std::unique_ptr<If> make_if(std::unique_ptr<ASTNode> && cond,
								std::unique_ptr<ASTNode> && statements,
								std::unique_ptr<ASTNode> && else_ = {});

	std::unique_ptr<While> make_while(std::unique_ptr<ASTNode> && cond,
									  std::unique_ptr<ASTNode> && statements);

	std::unique_ptr<For> make_for(std::unique_ptr<ASTNode> && left,
								  std::unique_ptr<ASTNode> && mid,
								  std::unique_ptr<ASTNode> && right,
								  std::unique_ptr<ASTNode> && statements);

	std::unique_ptr<VectorDecl> make_vector_decl(std::vector<std::unique_ptr<ASTNode>> && init_list);
	std::unique_ptr<VectorAccess> make_vector_access(std::unique_ptr<ASTNode> && vec,
													 std::unique_ptr<ASTNode> && index);
	
	std::unique_ptr<GlobalFunctionCall> make_global_fn_call(std::string && fn_name, std::vector<std::unique_ptr<ASTNode>> && params);

	std::unique_ptr<MemberFunctionCall> make_member_fn_call(
		std::string && fn_name,
		std::unique_ptr<ASTNode> && inst,
		std::vector<std::unique_ptr<ASTNode>> && params);

	std::unique_ptr<MemberVariableAccess> make_member_var_access(std::string name,
		std::unique_ptr<ASTNode> && inst);
}
