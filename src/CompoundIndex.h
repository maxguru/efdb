
#ifndef COMPOUND_INDEX_H
#define COMPOUND_INDEX_H

#include <typeinfo>
#include <boost/unordered_map.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

template<typename ModelClassPtr, typename... FieldTypes>
class CompoundIndex;

#include "HashIndex.h"
#include "KeyOperators.h"
#include "Field.h"

template<typename ModelClassPtr, typename... FieldTypes>
class CompoundIndex : public HashIndex< ModelClassPtr, boost::tuple< typename FieldTypes::type... > >
{
public:
		
		typedef boost::tuple< typename FieldTypes::type... > TupleType;
		typedef HashIndex< ModelClassPtr, TupleType > ParentClass;
		
		template<typename KeyType, std::size_t counter, typename... OtherFieldIds>
		inline bool updateTupleSubkeyHelper(FieldId updatingFieldId, TupleType& key, const boost::any& subkey, FieldId fieldId)
		{
			if(updatingFieldId == fieldId)
			{
				boost::tuples::get<counter>(key) = boost::any_cast<typename KeyType::head_type>(subkey);
				return true;
			}
			else
				return false;
		}
		
		template<typename KeyType, std::size_t counter, typename... OtherFieldIds>
		inline bool updateTupleSubkeyHelper(FieldId updatingFieldId, TupleType& key, const boost::any& subkey, FieldId fieldId, OtherFieldIds... otherFieldIds)
		{
			if(updateTupleSubkeyHelper<KeyType, counter>(updatingFieldId, key, subkey, fieldId))
				return true;
			else
				return updateTupleSubkeyHelper<typename KeyType::tail_type, counter+1, OtherFieldIds...>(updatingFieldId, key, subkey, otherFieldIds...);
		}
		
		template<typename... OtherFieldIds>
		inline void updateTupleSubkey(FieldId updatingFieldId, TupleType& key, const boost::any& subkey, FieldId fieldId, OtherFieldIds... otherFieldIds)
		{
			updateTupleSubkeyHelper<TupleType, 0, OtherFieldIds...>(updatingFieldId, key, subkey, fieldId, otherFieldIds...);
		}
		
		virtual void store(FieldId fieldId, const boost::any& subkey, ModelClassPtr instance)
		{
			// if trying to store full tuple key, do that
			if(subkey.type() == typeid(TupleType))
				return ParentClass::store(fieldId, subkey, instance);
			
			// otherwise, do partial key update
			
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			TupleType key;
			
			typename ParentClass::Multimap::right_iterator it = this->index.right.find(instance);
			if(it != this->index.right.end())
				key = it->second;
			
			updateTupleSubkey(fieldId, key, subkey, FieldTypes::field_id...);
			
			return ParentClass::store(fieldId, key, instance);
		}
		
		virtual bool isCompoundIndex() const { return true; }
};

namespace boost
{
	namespace tuples
	{
		namespace detail
		{
			template <class Tuple, size_t Index = length<Tuple>::value - 1>
			struct HashValueImpl
			{
				static void apply(size_t& seed, Tuple const& tuple)
				{
					HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
					// TODO: if a key is a pointer, we will probably want to hash the the contents, not the pointer itself
					// however, this is not the case with relation keys, we are ok comparing pointers to other models which are not supposed to be duplicated in memory
					// we need to figure out a way to selectively compare contents per field type
					//smart_hash_combine<decltype(get<Index>(tuple))>()(seed, get<Index>(tuple));
					
					boost::hash_combine(seed, get<Index>(tuple));
				}
			};
			template <class Tuple>
			struct HashValueImpl<Tuple,0>
			{
				static void apply(size_t& seed, Tuple const& tuple)
				{
					// TODO: if a key is a pointer, we will probably want to hash the the contents, not the pointer itself
					// however, this is not the case with relation keys, we are ok comparing pointers to other models which are not supposed to be duplicated in memory
					// we need to figure out a way to selectively compare contents per field type
					//smart_hash_combine<decltype(get<0>(tuple))>()(seed, get<0>(tuple));
					
					boost::hash_combine(seed, get<0>(tuple));
				}
			};
		}
		
		template <class Tuple>
		size_t hash_value(Tuple const& tuple)
		{
			size_t seed = 0;
			detail::HashValueImpl<Tuple>::apply(seed, tuple);
			return seed;
		}
	}
}

#endif /* COMPOUND_INDEX_H */
