
#ifndef LOCKABLE_H
#define LOCKABLE_H

#include <boost/smart_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/enable_shared_from_this.hpp>

class Lockable : public boost::enable_shared_from_this< Lockable >
{
	public:
		
		Lockable();
		virtual ~Lockable();
		
		virtual boost::shared_mutex& getMutex() const;
		
	private:
		
		mutable boost::shared_mutex mutex;
};

#endif /* LOCKABLE_H */
