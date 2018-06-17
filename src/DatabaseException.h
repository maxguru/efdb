#pragma once

#include <stdexcept>

class DatabaseException : public std::runtime_error
{
	public:
		DatabaseException(const std::string& message = "")  : runtime_error(message) {};
		virtual ~DatabaseException() throw() {};
};
