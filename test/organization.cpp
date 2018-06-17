#include "organization.h"
#include "db.h"

organization::organization() : secretary(PersonPtr()), accountant(NULL) { }
organization::organization(const std::string & name, PersonPtr founder, PersonPtr s) : group(name, founder), secretary(s), accountant(NULL) { }
