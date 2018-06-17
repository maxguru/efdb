
#ifndef RELATION_FIELD_H
#define RELATION_FIELD_H

template<typename RelationModelClassPtr, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies>
class RelationField;

#include "IndexedField.h"
#include "RelationIndex.h"

template<typename RelationModelClassPtr, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies>
class RelationField : public IndexedField<RelationModelClassPtr, fieldId, ModelClassPtr, fieldPolicies>
{
	typedef IndexedField<RelationModelClassPtr, fieldId, ModelClassPtr, fieldPolicies> parentClass;
	
	public:
		
		RelationField() { }
		RelationField(RelationModelClassPtr v)
		: parentClass(v)
		{ }
		
		virtual void registerField() const
		{
			typedef boost::shared_ptr< Index<ModelClassPtr> > IndexPtr;
			IndexPtr index(new RelationIndex<ModelClassPtr, RelationModelClassPtr>);
			ModelStoreGetter<ModelClassPtr>()().addIndex(index, fieldId);
			ModelStoreGetter<RelationModelClassPtr>()().registerRelationModelStore(&ModelStoreGetter<ModelClassPtr>()());
		}
		
		virtual const RelationModelClassPtr& operator=(const RelationModelClassPtr& rhs)
		{
			// check to make sure this instance has been stored
			// TODO: throw a better error message
			rhs->getId();
			
			// save old value
			RelationModelClassPtr old(this->var);
			
			// call parent function for processing
			const RelationModelClassPtr& retval(parentClass::operator=(rhs));
			
			// trigger automatic cleanup of old value if necessary
			if(old)
				old->doAutomaticCleanup();
			
			return retval;
		}
		
		virtual void reset()
		{
			// save old value
			RelationModelClassPtr old(this->var);
			
			// reset wrapped variable
			this->var.reset();
			
			// update index
			this->updateIndex();
			
			// trigger automatic cleanup of old value if necessary
			if(old)
				old->doAutomaticCleanup();
		}
		
		virtual void modelDeleteHandler()
		{
			if(this->var)
				this->var->doAutomaticCleanup();
		}
};

#endif /* RELATION_FIELD_H */
