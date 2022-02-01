#pragma once

#include "ScriptingBaseException.h"
#include "BoxedValue.h"

namespace except
{
	class RuntimeException : public ScriptingBaseException
	{
	public:
		explicit RuntimeException(std::string err) throw() : ScriptingBaseException{ std::move(err) } {}
	};

	class AssertionFailure : public ScriptingBaseException
	{
	public:
		explicit AssertionFailure(std::string err) throw() : ScriptingBaseException{ std::move(err) } {}
	};

	namespace impl
	{
		/// \brief	Will store an object of this type in a BoxedValue to 
		/// return an error without throwing and exception.
		struct RuntimeError {};
	}

	inline BoxedValue make_boxed_runtime_error() { return BoxedValue{ impl::RuntimeError{} }; }
	inline bool is_boxed_error(const BoxedValue & bv) { return bv.is_storing<impl::RuntimeError>(); }
}

// TODO(Borja): Parametrize these preprocesor ifs

// set to 1 to stop execution when an exception location is triggered, for debug purposes
#if 0
#	define _SCR_DEBUG_STOP_EXECUTION()		__debugbreak()
#else
#	define _SCR_DEBUG_STOP_EXECUTION()		do{}while(0)
#endif

// set to 1 to remove the error boilerplate (handy for release builds)
#if 0
#	define _SCR_RUNTIME_EXCEPTION(...)		throw ::except::RuntimeException{ "" }
#else

#include <sstream>
namespace except
{
	namespace impl
	{
		template <typename T, typename ... Ts>
		std::string concat_exception(T && t, Ts && ... vs)
		{
			std::stringstream ss;
			ss << t;
			return ss.str() + concat_exception(std::forward<Ts>(vs)...);
		}
		inline const char * concat_exception()
		{
			return "";
		}
	}
}

#	define _SCR_RUNTIME_EXCEPTION(...)	throw ::except::RuntimeException{ ::except::impl::concat_exception(__VA_ARGS__) }
#endif

#define SCR_RUNTIME_EXCEPTION(...)				\
	do											\
	{											\
		_SCR_DEBUG_STOP_EXECUTION();			\
		_SCR_RUNTIME_EXCEPTION(__VA_ARGS__);	\
	} while(0)
