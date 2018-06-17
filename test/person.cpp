#include "person.h"
#include "db.h"

person::person() : name(""), number(0) { }
person::person(const std::string & nam, int num) : name(nam), number(num) { }

unsigned int person::getNumber() { return number; }
void person::setNumber(unsigned int n) { number = n; }

std::string person::getName() { return name; }
void person::setName(const std::string& n) { name = n; }
