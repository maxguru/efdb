#pragma once

#include "../src/ModelStore.h"
#include "../src/Model.h"
#include "../src/RelationStore.h"
#include "../src/CompoundIndex.h"

#include "person.h"
#include "group.h"
#include "organization.h"

typedef RelationStore<PersonPtr, GroupPtr> PersonGroups;

class db
{
	public:
		
		db();
		
		void load();
		void save() const;
		
		PersonStore people;
		GroupStore groups;
		
		static db& getInstance();
		static db* instance;
};

template<>
struct ModelStoreGetter<PersonPtr>
{
	PersonStore& operator()() const
	{
		return db::getInstance().people;
	}
};

template<>
struct ModelStoreGetter<GroupPtr>
{
	GroupStore& operator()() const
	{
		return db::getInstance().groups;
	}
};
