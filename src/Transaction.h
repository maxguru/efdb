
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <iostream>
#include <stdexcept>
#include <set>
#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>

class Transaction;

typedef boost::shared_ptr<Transaction> TransactionPtr;

#include "Lockable.h"
#include "DeadlockException.h"

#define DEBUG_TRANSACTIONS false

class Transaction : public boost::enable_shared_from_this< Transaction >
{
public:
		
		static const unsigned int deadlockTimeout;
		
		typedef boost::unordered_map<boost::thread::id, Transaction*> Transactions;
		
		typedef boost::shared_lock<boost::shared_mutex> SharedLock;
		typedef boost::upgrade_lock<boost::shared_mutex> UpgradeLock;
		typedef boost::upgrade_to_unique_lock< boost::shared_mutex > UpgradedLock;
		typedef boost::unique_lock<boost::shared_mutex> ExclusiveLock;
		typedef boost::shared_ptr< SharedLock > SharedLockPtr;
		typedef boost::shared_ptr< UpgradeLock > UpgradeLockPtr;
		typedef boost::shared_ptr< UpgradedLock > UpgradedLockPtr;
		typedef boost::shared_ptr< ExclusiveLock > ExclusiveLockPtr;
		
		typedef boost::unordered_map<const Lockable*, SharedLockPtr> SharedLocks;
		typedef boost::unordered_map<const Lockable*, UpgradeLockPtr> UpgradeLocks;
		typedef boost::unordered_map<const Lockable*, UpgradedLockPtr> UpgradedLocks;
		typedef boost::unordered_map<const Lockable*, ExclusiveLockPtr> ExclusiveLocks;
		
		typedef boost::bimap<
			boost::bimaps::unordered_multiset_of<const void*>,
			boost::bimaps::unordered_multiset_of<const Lockable*>,
			boost::bimaps::unordered_set_of_relation<>
		> LockHistory;
		typedef LockHistory::value_type LockHistoryElementType;
		
		~Transaction()
		{
			// release all the locks
			BOOST_FOREACH(ExclusiveLocks::value_type& i, exclusiveLocks)
				i.second.reset();
			BOOST_FOREACH(UpgradedLocks::value_type& i, upgradedLocks)
				i.second.reset();
			BOOST_FOREACH(UpgradeLocks::value_type& i, upgradeLocks)
				i.second.reset();
			BOOST_FOREACH(SharedLocks::value_type& i, sharedLocks)
				i.second.reset();
			
			{
				boost::lock_guard<boost::mutex> guard(transactionsMutex);
				transactions.erase(boost::this_thread::get_id());
			}
			
			// save history of locks so next time we can lock all needed locks right away to avoid deadlocks
			{
				boost::lock_guard<boost::mutex> guard(historyMutex);
				
				BOOST_FOREACH(const ExclusiveLocks::value_type& i, exclusiveLocks)
				{
					LockHistoryElementType relation(transactionStartAddress, i.first);
					if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end())
					{
						sharedLockHistory.erase(relation);
						exclusiveLockHistory.insert(relation);
					}
				}
				BOOST_FOREACH(const UpgradedLocks::value_type& i, upgradedLocks)
				{
					LockHistoryElementType relation(transactionStartAddress, i.first);
					if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end())
					{
						sharedLockHistory.erase(relation);
						exclusiveLockHistory.insert(relation);
					}
				}
				BOOST_FOREACH(const SharedLocks::value_type& i, sharedLocks)
				{
					LockHistoryElementType relation(transactionStartAddress, i.first);
					if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end()
					&& sharedLockHistory.find(relation) == sharedLockHistory.end())
						sharedLockHistory.insert(relation);
				}
			}
			
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": ended" << std::endl << std::flush;
				std::cout << "shared history:" << sharedLockHistory.size() << ", exclusive history:" << exclusiveLockHistory.size() << std::endl << std::flush;
			}
		}
		
		static TransactionPtr startTransaction()
		{
			boost::thread::id threadId = boost::this_thread::get_id();
			
			TransactionPtr transaction;
			{
				boost::lock_guard<boost::mutex> guard(transactionsMutex);
				
				Transactions::iterator it = transactions.find(threadId);
				if(it != transactions.end())
					return it->second->shared_from_this();
			}
			
			void* transactionStartAddress = __builtin_extract_return_addr(__builtin_return_address(0));
			
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": starting, from " << transactionStartAddress << std::endl << std::flush;
			}
			
			transaction = TransactionPtr(new Transaction(transactionStartAddress));
			
			{
				boost::lock_guard<boost::mutex> guard(transactionsMutex);
				transactions[threadId] = transaction.get();
			}
			
			return transaction;
		}
		
		static void removeLockable(const Lockable* resource)
		{
			boost::lock_guard<boost::mutex> guard(historyMutex);
			sharedLockHistory.right.erase(resource);
			exclusiveLockHistory.right.erase(resource);
			
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard2(coutMutex);
				std::cout << "shared history:" << sharedLockHistory.size() << ", exclusive history:" << exclusiveLockHistory.size() << std::endl << std::flush;
			}
		}
		
		void getSharedLock(const Lockable* resource)
		{
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": trying to shared lock " << resource << " ... " << std::endl << std::flush;
			}
			
			if(threadId != boost::this_thread::get_id())
				throw std::runtime_error("Using transaction from the wrong thread");
			
			if(sharedLocks.find(resource) != sharedLocks.end()
			|| exclusiveLocks.find(resource) != exclusiveLocks.end()
			|| upgradedLocks.find(resource) != upgradedLocks.end())
			{
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": already have it" << std::endl << std::flush;
				}
				return;
			}
			
			SharedLockPtr lock(new SharedLock(resource->getMutex(), boost::defer_lock));
			if(!lock->try_lock_for(boost::chrono::seconds(deadlockTimeout)))
			{
				boost::lock_guard<boost::mutex> guard(historyMutex);
				LockHistoryElementType relation(transactionStartAddress, resource);
				if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end()
				&& sharedLockHistory.find(relation) == sharedLockHistory.end())
					sharedLockHistory.insert(relation);
				
				throw DeadlockException();
			}
			
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": got shared lock" << std::endl << std::flush;
			}
			
			sharedLocks[resource] = lock;
		}
		
		void getExclusiveLock(const Lockable* resource)
		{
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": trying to exclusive lock " << resource << " ... " << std::endl << std::flush;
			}
			
			if(threadId != boost::this_thread::get_id())
				throw std::runtime_error("Using transaction from the wrong thread");
			
			if(exclusiveLocks.find(resource) != exclusiveLocks.end()
			|| upgradedLocks.find(resource) != upgradedLocks.end())
			{
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": already have it" << std::endl << std::flush;
				}
				
				return;
			}
			if(sharedLocks.find(resource) != sharedLocks.end())
			{
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": trying to get upgrade lock... " << std::endl << std::flush;
				}
				
				UpgradeLockPtr lock(new UpgradeLock(resource->getMutex(), boost::defer_lock));
				if(!lock->try_lock_for(boost::chrono::seconds(deadlockTimeout)))
				{
					boost::lock_guard<boost::mutex> guard(historyMutex);
					LockHistoryElementType relation(transactionStartAddress, resource);
					if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end())
					{
						sharedLockHistory.erase(relation);
						exclusiveLockHistory.insert(relation);
					}
					
					throw DeadlockException();
				}
				
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": got upgrade lock, " << std::endl << std::flush;
				}
				
				upgradeLocks[resource] = lock;
				
				// free shared lock, we have the upgrade lock now
				sharedLocks.erase(resource);
				
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": trying to upgrade... " << std::endl << std::flush;
				}
				
				// upgrade the lock
				// TODO: need deadlock detection here
				UpgradedLockPtr upgradedLock(new UpgradedLock(*lock));
				
				if(DEBUG_TRANSACTIONS)
				{
					boost::lock_guard<boost::mutex> guard(coutMutex);
					std::cout << "transaction " << threadId << ": got upgraded lock" << std::endl << std::flush;
				}
				
				upgradedLocks[resource] = upgradedLock;
				
				return;
			}
			
			ExclusiveLockPtr lock(new ExclusiveLock(resource->getMutex(), boost::defer_lock));
			if(!lock->try_lock_for(boost::chrono::seconds(deadlockTimeout)))
			{
				boost::lock_guard<boost::mutex> guard(historyMutex);
				LockHistoryElementType relation(transactionStartAddress, resource);
				if(exclusiveLockHistory.find(relation) == exclusiveLockHistory.end())
				{
					sharedLockHistory.erase(relation);
					exclusiveLockHistory.insert(relation);
				}
				
				throw DeadlockException();
			}
			
			if(DEBUG_TRANSACTIONS)
			{
				boost::lock_guard<boost::mutex> guard(coutMutex);
				std::cout << "transaction " << threadId << ": got exclusive lock" << std::endl << std::flush;
			}
			
			exclusiveLocks[resource] = lock;
		}
		
	private:
		
		Transaction(void* addr) : threadId(boost::this_thread::get_id()), transactionStartAddress(addr)
		{
			// lock all the resources preemptively that were needed last time a transaction was started from this address
			// lock resources in order of increasing memory address to minimize collisions
			
			std::set<const Lockable*> sharedResources, exclusiveResources;
			
			{
				boost::lock_guard<boost::mutex> guard(historyMutex);
				
				BOOST_FOREACH(LockHistory::left_const_reference& i, exclusiveLockHistory.left.equal_range(transactionStartAddress))
					exclusiveResources.insert(i.second);
				BOOST_FOREACH(LockHistory::left_const_reference& i, sharedLockHistory.left.equal_range(transactionStartAddress))
					sharedResources.insert(i.second);
			}
			
			if(exclusiveResources.size() + sharedResources.size() > 1)
			{
				const Lockable* failed = NULL;
				bool failedShared = false;
				do
				{
					exclusiveLocks.clear();
					sharedLocks.clear();
					
					if(failed)
					{
						if(failedShared)
						{
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptively waiting on shared lock " << failed << "..." << std::endl << std::flush;
							}
							
							SharedLockPtr lock(new SharedLock(failed->getMutex()));
							
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptively got shared lock " << failed << " on retry" << std::endl << std::flush;
							}
							
							sharedLocks[failed] = lock;
						}
						else
						{
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptively waiting on exclusive lock " << failed << "..." << std::endl << std::flush;
							}
							
							ExclusiveLockPtr lock(new ExclusiveLock(failed->getMutex()));
							
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptively got exclusive lock " << failed << " on retry" << std::endl << std::flush;
							}
							
							exclusiveLocks[failed] = lock;
						}
						failed = NULL;
					}
					
					BOOST_FOREACH(const std::set<const Lockable*>::value_type& resource, exclusiveResources)
					{
						if(exclusiveLocks.find(resource) != exclusiveLocks.end()
						|| sharedLocks.find(resource) != sharedLocks.end())
							continue;
						
						ExclusiveLockPtr lock(new ExclusiveLock(resource->getMutex(), boost::try_to_lock));
						
						if(!lock->owns_lock())
						{
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptive exclusive locking failed on " << resource << std::endl << std::flush;
							}
							
							failed = resource;
							failedShared = false;
							break;
						}
						
						if(DEBUG_TRANSACTIONS)
						{
							boost::lock_guard<boost::mutex> guard(coutMutex);
							std::cout << "transaction " << threadId << ": preemptively got exclusive lock " << resource << std::endl << std::flush;
						}
						
						exclusiveLocks[resource] = lock;
					}
					
					if(failed)
						continue;
					
					BOOST_FOREACH(const std::set<const Lockable*>::value_type& resource, sharedResources)
					{
						if(exclusiveLocks.find(resource) != exclusiveLocks.end()
						|| sharedLocks.find(resource) != sharedLocks.end())
							continue;
						
						SharedLockPtr lock(new SharedLock(resource->getMutex(), boost::try_to_lock));
						
						if(!lock->owns_lock())
						{
							if(DEBUG_TRANSACTIONS)
							{
								boost::lock_guard<boost::mutex> guard(coutMutex);
								std::cout << "transaction " << threadId << ": preemptive shared locking failed on " << resource << std::endl << std::flush;
							}
							
							failed = resource;
							failedShared = true;
							break;
						}
						
						if(DEBUG_TRANSACTIONS)
						{
							boost::lock_guard<boost::mutex> guard(coutMutex);
							std::cout << "transaction " << threadId << ": preemptively got shared lock " << resource << std::endl << std::flush;
						}
						
						sharedLocks[resource] = lock;
					}
				} while(failed);
			}
		}
		
		// disable copying
		Transaction(const Transaction&);
		Transaction& operator=(const Transaction&);
		
		static boost::mutex coutMutex;
		
		static boost::mutex transactionsMutex;
		static Transactions transactions;
		
		boost::thread::id threadId;
		SharedLocks sharedLocks;
		UpgradeLocks upgradeLocks;
		UpgradedLocks upgradedLocks;
		ExclusiveLocks exclusiveLocks;
		
		const void* transactionStartAddress;
		static boost::mutex historyMutex;
		static LockHistory sharedLockHistory;
		static LockHistory exclusiveLockHistory;
};

#define TRANSACTION_H_DONE

#endif /* TRANSACTION_H */
