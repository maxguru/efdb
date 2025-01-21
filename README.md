![EFDB Logo](assets/efdb-logo.svg)

# Extremely Fast Database for C++ Applications - efdb

EFDB is a header-only C++ library implementing an extremely fast in-memory ORM-like database. The goal of this library is to simplify the development of C++ applications that work with relational data using object-oriented programming methodology without the overhead of object serialization.

**WARNING**: This project is still in early development and is not yet ready for production use.

## Usage

EFDB is an embedded field database that provides a simple yet powerful way to store and query data with indexed fields.

### Basic Operations

```cpp
// Initialize database
db database;
database.load();

// Access stores
PersonStore& people = database.people;
GroupStore& groups = database.groups;

// Create and store a new person
PersonPtr person(new person("bob", 1));
person->store();

// Query by indexed field
PersonPtr found = people.get<person::NAME>("bob");

// Update fields
person->setName("bob junior");

// Delete an entry
person->erase();
```

### Querying Data

```cpp
// Get all records
PersonStore::ModelListPtr person_list = people.getList();

// Query by field value
person_list = people.getList<person::NUMBER>(1);

// Query by multiple fields
PersonPtr person = people.get<person::NAME, person::NUMBER>("steve", 2);

// Get list of IDs
PersonStore::IDList idlist = people.getIdList();
```

### Relationships and Complex Types

```cpp
// Create related entities
PersonPtr founder(new person("founder", 256));
founder->store();
GroupPtr group(new group("somegroup", founder));
group->store();

// Query by relationship
GroupStore::ModelListPtr founded_groups = groups.getList<group::FOUNDER>(founder);
```

## Data Model Definition

To define a model with indexed fields:

```cpp
class person : public Model<PersonPtr>
{
	MODEL_FRIENDS
	DO_AUTOMATIC_CLEANUP
	
	public:
		static const FieldId NAME = 1;
		static const FieldId NUMBER = 2;
		
	private:
		IndexedField<std::string, NAME, PersonPtr> name;
		IndexedField<unsigned int, NUMBER, PersonPtr> number;
};

// Register model fields
REGISTER_MODEL_FIELD_IDS(PersonPtr, person, person::NAME, person::NUMBER)
REGISTER_MODEL_FIELD(PersonPtr, person::NAME, person::name)
REGISTER_MODEL_FIELD(PersonPtr, person::NUMBER, person::number)
```

### Automatic Cleanup

The library handles automatic cleanup of related entities and maintains referential integrity when deleting objects.

### Inheritance Support

The library supports class inheritance. Here's an example of a base class and its derived class:

```cpp
// Base class
class group : public Model<GroupPtr>
{
	MODEL_FRIENDS
	
	public:
		static const FieldId NAME = 1;
		static const FieldId FOUNDER = 2;
		
	private:
		IndexedField<std::string, NAME, GroupPtr> name;
		RelationField<PersonPtr, FOUNDER, GroupPtr, FieldPolicies::OnDeleteErase> founder;
};

// Register base class fields
REGISTER_MODEL_FIELD_IDS(GroupPtr, group, group::NAME, group::FOUNDER)
REGISTER_MODEL_FIELD(GroupPtr, group::NAME, group::name)
REGISTER_MODEL_FIELD(GroupPtr, group::FOUNDER, group::founder)

// Derived class with additional fields
class organization : public group
{
	MODEL_FRIENDS
	
	public:
		static const FieldId SECRETARY = 10;
		static const FieldId ACCOUNTANT = 11;
		
	private:
		// Different deletion policy for these relations
		RelationField<PersonPtr, SECRETARY, GroupPtr, FieldPolicies::OnDeleteSetToNull> secretary;
		RelationField<PersonPtr, ACCOUNTANT, GroupPtr, FieldPolicies::OnDeleteSetToNull> accountant;
};

// Register inheritance relationship
REGISTER_PARENT_MODEL(organization, group)
// Register derived class fields
REGISTER_MODEL_FIELD_IDS(GroupPtr, organization, organization::SECRETARY, organization::ACCOUNTANT)
REGISTER_MODEL_FIELD(GroupPtr, organization::SECRETARY, organization::secretary)
REGISTER_MODEL_FIELD(GroupPtr, organization::ACCOUNTANT, organization::accountant)

// Usage example
PersonPtr founder(new person("founder", 1));
PersonPtr secretary(new person("secretary", 2));
founder->store();
secretary->store();

// Create and store an organization
GroupPtr company = GroupPtr(new organization("ACME Corp", founder, secretary));
company->store();

// Access derived class features
boost::static_pointer_cast<organization>(company)->setSecretary(secretary);
```

### Thread Safety

The database supports concurrent access with proper locking mechanisms and deadlock detection. Multiple threads can safely perform operations on the database simultaneously.

### Transaction Support

```cpp
// Start a transaction with locks
TransactionPtr transaction = Transaction::startTransaction();
transaction->getSharedLock(&people);
transaction->getExclusiveLock(&people);
```

### JSON Serialization

```cpp
// Convert objects to JSON
std::string personJson = *(people.toJson(person));
std::string groupJson = *(groups.toJson(group));
```

### Database Persistence

EFDB provides JSON-based persistence to save and load the database state to/from the filesystem. Each model store can be independently serialized to and deserialized from JSON files:

```cpp
// Manual store import/export
people.importJson("people.json");
people.exportJson("people.json");

// Import/export with transaction safety
TransactionPtr transaction = Transaction::startTransaction();
transaction->getExclusiveLock(&people);
people.importJson("custom_people.json");
transaction.reset();
transaction = Transaction::startTransaction();
transaction->getSharedLock(&groups);
groups.exportJson("groups.json");
```

## Supporting EFDB

If you find the idea behind EFDB valuable, please consider supporting its development.

### Financial Support

You can financially support EFDB's development through the [GitHub Sponsors page](https://github.com/sponsors/maxguru). Your sponsorship will:
- Motivate me to finish the initial release of the library
- Enable me to continue to improve the library, fix bugs and develop new features
- Provide funds for working on improving documentation and addressing design issues

### Contributing

We also welcome contributions through:
- Bug reports and feature requests
- Documentation improvements
- Pull requests
- Testing and feedback

Thank you for your contributions!
