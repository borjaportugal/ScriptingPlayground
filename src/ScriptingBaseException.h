#pragma once

#include <exception>	// std::exception
#include <string>		// std::string

namespace except
{
	class ScriptingBaseException : public std::exception
	{
	public:
		// TODO(Borja): pass info about file and line
		explicit ScriptingBaseException(std::string && err) throw() : m_err{ std::move(err) } {}

		const char * what() const throw() override { return m_err.c_str(); }

	private:
		const std::string m_err;
	};
}

