
#ifndef INDEXED_FIELD_H
#define INDEXED_FIELD_H

#include <cstddef>

#include "Field.h"

template<typename FieldType, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies = FieldPolicies::None>
class IndexedField;

#include "Model.h"
#include "HashIndex.h"

template<typename FieldType, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies>
class IndexedField : public Field<FieldType, fieldId, ModelClassPtr, fieldPolicies>
{
	typedef Field<FieldType, fieldId, ModelClassPtr, fieldPolicies> parentClass;
	
	public:
		
		IndexedField() { }
		IndexedField(const FieldType& v)
		: parentClass(v)
		{ }
		
		virtual void registerField() const
		{ }
		
		virtual const FieldType& operator=(const FieldType& rhs)
		{
			// TODO: it may be a bit too expensive to lock on assignment, is it really necessary?
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(static_cast<Lockable*>(&ModelStoreGetter<ModelClassPtr>()()));
			
			this->var = rhs;
			updateIndex();
			
			return this->var;
		}
		
		virtual void updateIndex() const
		{
			ModelStoreGetter<ModelClassPtr>()().template updateIndex<FieldType>(this->getModel(), fieldId, this->var);
		}
};

#endif /* INDEXED_FIELD_H */
