#include "db.h"
#include "person.h"

using namespace boost::filesystem;

db* db::instance(NULL);

db::db()
	: people()
	, groups()
{
	instance = this;
	
	people.registerModel<person>();
	people.addIndex<person::NAME>();
	people.addIndex<person::NUMBER>();
	people.addCompoundIndex<person::NAME, person::NUMBER>();
	
	groups.registerModel<group>();
	groups.registerModel<organization>();
	groups.addIndex<group::NAME>();
	groups.addCompoundIndex<group::FOUNDER, organization::SECRETARY>();
}

db& db::getInstance()
{
	return *instance;
}

void db::load()
{
	TransactionPtr transaction = Transaction::startTransaction();
	transaction->getExclusiveLock(&people);
	transaction->getExclusiveLock(&groups);
	
	if(exists(path("people.json"))) people.importJson("people.json");
	if(exists(path("groups.json"))) groups.importJson("groups.json");
}

void db::save() const
{
	TransactionPtr transaction = Transaction::startTransaction();
	transaction->getSharedLock(&people);
	transaction->getSharedLock(&groups);
	
	people.exportJson("people.json");
	groups.exportJson("groups.json");
}
