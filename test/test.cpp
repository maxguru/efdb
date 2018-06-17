#include <string>
#include <iostream>
using namespace std;

#include "db.h"

volatile bool run = true;

int main2()
{
	while(run)
	{
		PersonPtr p(new person("incognito",255));
		p->store();
		p->erase();
	}
	return 0;
}

int main3()
{
	while(run)
	{
		PersonPtr p(new person("founder",256));
		p->store();
		GroupPtr g(new group("somegroup", p));
		g->store();
		g->erase();
		p->erase();
	}
	return 0;
}

int main()
{
	db database;
	
	database.load();
	
	PersonStore& people(database.people);
	GroupStore& groups(database.groups);
	PersonGroups& persons_groups(database.persons_groups);
	
	PersonPtr bob, john, bob2, steve, bill;
	GroupPtr smiths, club_members, the_company, bill_group;
	
	try { bob = people.get<person::NAME>("bob"); } catch(const InstanceNotFoundException& e) { (bob = PersonPtr(new person("bob",1)))->store(); }
	try { bob2 = people.get<person::NAME>("bob 2"); } catch(const InstanceNotFoundException& e) { (bob2 = PersonPtr(new person("bob 2",1)))->store(); }
	try { john = people.get<person::NAME>("john"); } catch(const InstanceNotFoundException& e) { (john = PersonPtr(new person("john",3)))->store(); }
	try { steve = people.get<person::NAME>("steve"); } catch(const InstanceNotFoundException& e) { (steve = PersonPtr(new person("john",2)))->store(); }
	try { bill = people.get<person::NAME>("bill"); } catch(const InstanceNotFoundException& e) { (steve = PersonPtr(new person("bill",5)))->store(); }
	
	try { smiths = groups.get<group::NAME>("smiths"); } catch(const InstanceNotFoundException& e) { (smiths = GroupPtr(new group("smiths", bob2)))->store(); }
	try { club_members = groups.get<group::NAME>("club members"); } catch(const InstanceNotFoundException& e) { (club_members = GroupPtr(new group("club members", steve)))->store(); }
	try { the_company = groups.get<group::NAME>("the company"); } catch(const InstanceNotFoundException& e) { (the_company = GroupPtr(new organization("the company", steve, bob2)))->store(); }
	try { bill_group = groups.get<group::NAME>("bill group"); } catch(const InstanceNotFoundException& e) { (bill_group = GroupPtr(new group("bill group", bill)))->store(); }
	
	//persons_groups.store(bob, smiths);
	//persons_groups.store(bob2, smiths);
	//persons_groups.store(bob, club_members);
	//persons_groups.store(bob2, club_members);
	//persons_groups.store(steve, club_members);
	
	database.save();
	
	cout << "id list," << endl;
	PersonStore::IDList idlist = people.getIdList();
	BOOST_FOREACH(const PersonStore::ID& i, *idlist)
		cout << i << endl;
	cout << endl;
	
	PersonStore::ModelListPtr person_list;
	
	cout << "people," << endl;
	person_list = people.getList();
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << *(i->toJson()) << endl;
	cout << endl;
	
	GroupStore::ModelListPtr group_list;
	
	cout << "groups," << endl;
	group_list = groups.getList();
	BOOST_FOREACH(const GroupStore::ValueType& i, *group_list)
		cout << i << " " << *(i->toJson()) << endl;
	cout << endl;
	
	cout << "people with number = 1," << endl;
	person_list = people.getList<person::NUMBER>(1);
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << "deleting " << bob->getName() << "..." << endl;
	bob->erase();
	cout << "updating " << bob2->getName() << "..." << endl;
	bob2->setName("bob junior");
	cout << endl;
	
	boost::thread tempThread(main2);
	boost::thread tempThread1(main2);
	boost::thread tempThread2(main2);
	boost::thread tempThread3(main3);
	boost::thread tempThread4(main3);
	boost::thread tempThread5(main3);
	
	usleep(1000000);
	
	{
		while(true)
			try
			{
				TransactionPtr transaction = Transaction::startTransaction();
				transaction->getSharedLock(&people);
				transaction->getExclusiveLock(&people);
				break;
			}
			catch(const DeadlockException& e)
			{
				cout << "DEADLOCK DETECTED!" << endl;
			}
	}
	
	run = false;
	
	tempThread.join();
	tempThread1.join();
	tempThread2.join();
	tempThread3.join();
	tempThread4.join();
	tempThread5.join();
	
	cout << "people," << endl;
	person_list = people.getList();
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << "people with number = 1," << endl;
	person_list = people.getList<person::NUMBER>(1);
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << "steve's number: ";
	cout << people.get<person::NAME>("steve")->getNumber() << endl;
	cout << endl;
	
	cout << "person with number '1': ";
	cout << people.get<person::NUMBER>(1)->getName() << endl;
	cout << endl;
	
	cout << smiths->getName() << "," << endl;
	person_list = persons_groups.getList(smiths);
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << club_members->getName() << "," << endl;
	person_list = persons_groups.getList(club_members);
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << bob2->getName() << "'s groups," << endl;
	group_list = persons_groups.getList(bob2);
	BOOST_FOREACH(const GroupStore::ValueType& i, *group_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << bob2->getName() << " founded the following groups," << endl;
	group_list = groups.getList<group::FOUNDER>(bob2);
	BOOST_FOREACH(const GroupStore::ValueType& i, *group_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << bob2->getName() << " in JSON," << endl;
	cout << *(people.toJson(bob2)) << endl;
	cout << endl;
	
	cout << smiths->getName() << " in JSON," << endl;
	cout << *(groups.toJson(smiths)) << endl;
	cout << endl;
	
	cout << the_company->getName() << " in JSON," << endl;
	cout << *(groups.toJson(the_company)) << endl;
	cout << endl;
	
	cout << "deleting " << bob2->getName() << "..." << endl;
	bob2->erase();
	cout << endl;
	
	cout << "does " << bob2->getName() << " have references in the db?  " << people.hasRelationReferences(bob2) << endl;
	cout << "does " << steve->getName() << " have references in the db?  " << people.hasRelationReferences(steve) << endl;
	cout << endl;
	
	cout << "groups after deleting " << bob2->getName() << "," << endl;
	group_list = groups.getList();
	BOOST_FOREACH(const GroupStore::ValueType& i, *group_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << "deleting " << bill_group->getName() << "..." << endl;
	bill_group->erase();
	cout << endl;
	
	cout << "people after deleting " << bill_group->getName() << "," << endl;
	person_list = people.getList();
	BOOST_FOREACH(const PersonStore::ValueType& i, *person_list)
		cout << i << " " << i->getName() << endl;
	cout << endl;
	
	cout << the_company->getName() << " after deleting " << bob2->getName() << "," << endl;
	cout << *(groups.toJson(the_company)) << endl;
	cout << endl;
	
	cout << "person named 'steve' with number 2, " << endl;
	cout << *people.toJson(people.get<person::NAME, person::NUMBER>("steve", 2)) << endl;
	cout << endl;
	
	boost::static_pointer_cast<organization>(the_company)->setSecretary(steve);
	
	cout << "group with founder '" << steve->getName() << "' and secretary '" << steve->getName() << "', " << endl;
	cout << *groups.toJson(groups.get<group::FOUNDER, organization::SECRETARY>(steve, steve)) << endl;
	cout << endl;
	
	return 0;
}

