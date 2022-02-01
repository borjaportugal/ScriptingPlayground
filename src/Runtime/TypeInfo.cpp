
#include "TypeInfo.h"

namespace impl
{
	TypeInfo::id_type generate_unique_id()
	{
		static TypeInfo::id_type s_ids = 0;
		return ++s_ids;
	}
}

