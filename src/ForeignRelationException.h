#pragma once

#include "DatabaseException.h"

class ForeignRelationException : public DatabaseException
{
	public:
		ForeignRelationException() : DatabaseException("Foreign relation exception") { };
	private:
};
