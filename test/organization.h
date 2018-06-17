#pragma once

#include <string>

#include "group.h"

class organization;

class organization : public group
{
	MODEL_FRIENDS
	
	public:
		
		organization();
		organization(const std::string & name, PersonPtr founder, PersonPtr secretary);
		
		void setSecretary(PersonPtr p) { secretary = p; };
		
		static const FieldId SECRETARY = 10;
		static const FieldId ACCOUNTANT = 11;
		
	private:
		
		RelationField<PersonPtr, SECRETARY, GroupPtr, FieldPolicies::OnDeleteSetToNull> secretary;
		RelationField<PersonPtr, ACCOUNTANT, GroupPtr, FieldPolicies::OnDeleteSetToNull> accountant;
};

REGISTER_PARENT_MODEL(organization, group)
REGISTER_MODEL_FIELD_IDS(GroupPtr, organization, organization::SECRETARY, organization::ACCOUNTANT)
REGISTER_MODEL_FIELD(GroupPtr, organization::SECRETARY, organization::secretary)
REGISTER_MODEL_FIELD(GroupPtr, organization::ACCOUNTANT, organization::accountant)
