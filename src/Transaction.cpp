#include "Transaction.h"

const unsigned int Transaction::deadlockTimeout = 300;

boost::mutex Transaction::coutMutex;

boost::mutex Transaction::transactionsMutex;
Transaction::Transactions Transaction::transactions;

boost::mutex Transaction::historyMutex;
Transaction::LockHistory Transaction::sharedLockHistory;
Transaction::LockHistory Transaction::exclusiveLockHistory;
