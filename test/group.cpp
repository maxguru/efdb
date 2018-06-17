#include "group.h"
#include "db.h"

group::group() : name(""), founder(PersonPtr()) { }
group::group(const std::string & nam, PersonPtr f) : name(nam), founder(f) { }

std::string group::getName() { return name; }
void group::setName(const std::string& n) { name = n; }
