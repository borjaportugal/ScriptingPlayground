
#include "Bindings.h"
#include "RuntimeException.h"

#include <algorithm>	// std::sort

namespace binds
{
	namespace impl
	{
		template <typename T>
		const T * find_best_overload(
			const std::vector<std::unique_ptr<T>> & overloads,
			runtime::DispatchEngine & en,
			std::vector<BoxedValue> & args)
		{
			// TODO(Borja): short the overloads by the parameter number they take and then only loop through 
			// the ones that have exactly the parameter number equal to args.size()

			binds::impl::FunctionCallMatchScore best_score = binds::impl::FunctionCallMatchScore::invalid;
			std::size_t call_idx = overloads.size();
			for (std::size_t i = 0; i < overloads.size(); ++i)
			{
				const auto score = overloads[i]->get_call_score(en, args);
				if (score > best_score)
				{
					call_idx = i;
					best_score = score;
				}
			}

			if (call_idx < overloads.size())
				return overloads[call_idx].get();

			return nullptr;
		}
	}

	OverloadedGlobalFunctionBinding::OverloadedGlobalFunctionBinding(
		std::unique_ptr<GlobalFunctionBinding> && overload0,
		std::unique_ptr<GlobalFunctionBinding> && overload1)
	{
		m_overloads.reserve(2);
		add_overload(std::move(overload0));
		add_overload(std::move(overload1));
	}

	BoxedValue OverloadedGlobalFunctionBinding::do_call(runtime::DispatchEngine & en,
		std::vector<BoxedValue> & args) const
	{
		if (auto * fn = impl::find_best_overload(m_overloads, en, args))
			return fn->do_call(en, args);

		SCR_RUNTIME_EXCEPTION("Could not find a valid overload.");
	}

	void OverloadedGlobalFunctionBinding::add_overload(std::unique_ptr<GlobalFunctionBinding> && overload)
	{
		m_overloads.emplace_back(std::move(overload));
	}

	OverloadedMemberFunctionBinding::OverloadedMemberFunctionBinding(
		std::unique_ptr<MemberFunctionBinding> && overload0,
		std::unique_ptr<MemberFunctionBinding> && overload1)
	{
		m_overloads.reserve(2);
		add_overload(std::move(overload0));
		add_overload(std::move(overload1));
	}

	BoxedValue OverloadedMemberFunctionBinding::do_call(runtime::DispatchEngine & en,
		BoxedValue & inst, std::vector<BoxedValue> & args) const
	{
		if (auto * fn = impl::find_best_overload(m_overloads, en, args))
			return fn->do_call(en, inst, args);

		SCR_RUNTIME_EXCEPTION("Could not find a valid overload.");
	}

	void OverloadedMemberFunctionBinding::add_overload(std::unique_ptr<MemberFunctionBinding> && overload)
	{
		m_overloads.emplace_back(std::move(overload));
	}
}
