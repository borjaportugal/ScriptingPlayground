
#include "Runtime\Operators.h"

#include <iostream>

namespace binds
{
	const BinaryOperators::operation_fn * BinaryOperators::get_operator(const TypeInfo & lhs_type,
																OperatorType op,
																const TypeInfo & rhs_type) const
	{
		const auto it = m_all_operations.find(op);
		if (it != m_all_operations.end())
		{
			const auto it2 = it->second.find(get_type_pair_hash(lhs_type, rhs_type));
			if (it2 != it->second.end())
				return &it2->second;
		}

		return nullptr;
	}
}
