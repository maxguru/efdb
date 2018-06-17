#pragma once

#include "DatabaseException.h"

class DeadlockException : public DatabaseException
{
	public:
		DeadlockException() : DatabaseException("Deadlock detected") { };
	private:
};
