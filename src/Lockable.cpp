#include "Lockable.h"
#include "Transaction.h"

Lockable::Lockable()
{
	
};

Lockable::~Lockable()
{
	Transaction::removeLockable(this);
}

boost::shared_mutex& Lockable::getMutex() const
{
	return mutex;
}
