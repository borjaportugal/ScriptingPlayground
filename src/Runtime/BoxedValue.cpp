
#include "BoxedValue.h"

BadBoxedCast::BadBoxedCast(const std::type_info & from, const std::type_info & to)
	: m_what{ std::string{ "Invalid boxed cast from " } + from.name() + " to " + to.name() }
{}

BoxedValue::BoxedValue(const BoxedValue & other)
	: m_boxed_value{ other.m_boxed_value->clone() }
{}

BoxedValue & BoxedValue::operator=(const BoxedValue & rhs)
{
	if (this != &rhs)
		m_boxed_value = rhs.m_boxed_value->clone();
	return *this;
}

// STUDY(Borja): in the future problems may arise when we need to return a const reference...
BoxedValue & resolve_ref(BoxedValue & bv)
{
	// if we are storing a 'reference' to an other BoxedValue, return actual BoxedValue
	if (bv.is_storing<BoxedValue>())
		return bv.get_as<BoxedValue>();

	return bv;
}
const BoxedValue & resolve_ref(const BoxedValue & bv)
{
	// if we are storing a 'reference' to an other BoxedValue, return actual BoxedValue
	if (bv.is_storing<BoxedValue>())
		return bv.get_as<BoxedValue>();

	return bv;
}

