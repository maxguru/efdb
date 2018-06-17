#pragma once

#include "../src/Model.h"

#include <string>
#include <boost/smart_ptr.hpp>

class person;

typedef boost::shared_ptr<person> PersonPtr;
typedef ModelStore<PersonPtr> PersonStore;

class person : public Model<PersonPtr>
{
	MODEL_FRIENDS
	DO_AUTOMATIC_CLEANUP
	
	public:
		
		person(const std::string & nam, int num);
		
		unsigned int getNumber();
		void setNumber(unsigned int n);
		
		std::string getName();
		void setName(const std::string& n);
		
		static const FieldId NAME = 1;
		static const FieldId NUMBER = 2;
		
	private:
		
		person();
		
		IndexedField<std::string, NAME, PersonPtr> name;
		IndexedField<unsigned int, NUMBER, PersonPtr> number;
};

REGISTER_MODEL_FIELD_IDS(PersonPtr, person, person::NAME, person::NUMBER)
REGISTER_MODEL_FIELD(PersonPtr, person::NAME, person::name)
REGISTER_MODEL_FIELD(PersonPtr, person::NUMBER, person::number)

