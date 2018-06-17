#pragma once

#include "DatabaseException.h"

class InstanceNotFoundException : public DatabaseException
{
	public:
		InstanceNotFoundException(std::string modelClass, std::string instance) : DatabaseException("Instance '" + instance + "' of type '" + modelClass + "' not found") { };
	private:
};
