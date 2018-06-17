#pragma once

#include <string>

#include "person.h"

class group;

typedef boost::shared_ptr<group> GroupPtr;
typedef ModelStore<GroupPtr> GroupStore;

class group : public Model<GroupPtr>
{
	MODEL_FRIENDS
	
	public:
		
		group();
		group(const std::string & nam, PersonPtr f);
		
		std::string getName();
		void setName(const std::string& n);
		
		static const FieldId NAME = 1;
		static const FieldId FOUNDER = 2;
		
	private:
		
		IndexedField<std::string, NAME, GroupPtr> name;
		RelationField<PersonPtr, FOUNDER, GroupPtr, FieldPolicies::OnDeleteErase> founder;
};

REGISTER_MODEL_FIELD_IDS(GroupPtr, group, group::NAME, group::FOUNDER)
REGISTER_MODEL_FIELD(GroupPtr, group::NAME, group::name)
REGISTER_MODEL_FIELD(GroupPtr, group::FOUNDER, group::founder)
