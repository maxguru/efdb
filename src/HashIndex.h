
#ifndef HASH_INDEX_H
#define HASH_INDEX_H

#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>

template<
	typename ModelClassPtr,
	typename KeyType,
	class KeyHashFunctor = boost::hash< BOOST_DEDUCED_TYPENAME 
		::boost::bimaps::tags::support::value_type_of<KeyType>::type >,
	class EqualKey = std::equal_to< BOOST_DEDUCED_TYPENAME 
		::boost::bimaps::tags::support::value_type_of<KeyType>::type >
>
class HashIndex;

#include "Transaction.h"
#include "InstanceNotFoundException.h"
#include "ModelStore.h"
#include "Index.h"
#include "KeyOperators.h"

template<
	typename ModelClassPtr,
	typename KeyType,
	class KeyHashFunctor,
	class EqualKey
>
class HashIndex : public Index<ModelClassPtr>, public Lockable
{
	public:
		
		typedef ModelClassPtr ValueType;
		
		typedef boost::bimap<
			boost::bimaps::unordered_multiset_of< KeyType, KeyHashFunctor, EqualKey >,
			boost::bimaps::unordered_multiset_of< ModelClassPtr, value_key_operators::hash<ModelClassPtr>, value_key_operators::equality<ModelClassPtr> >
		> Multimap;
		typedef typename Multimap::value_type IndexElementType;
		
		typedef typename ModelStore<ModelClassPtr>::ModelListPtr ModelListPtr;
		
		HashIndex() { };
		virtual ~HashIndex() { };
		
		virtual bool matchKeyType(const boost::any& key)
		{
			return key.type() == typeid(KeyType);
		}
		
		virtual void store(FieldId id, const boost::any& key, ModelClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			index.right.erase(instance);
			index.insert(IndexElementType(boost::any_cast<const KeyType>(key), instance));
		}
		
		virtual void erase(ModelClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			index.right.erase(instance);
		}
		
		virtual void clear()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			index.right.clear();
		}
		
		virtual ModelListPtr getList(const boost::any& key) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			ModelListPtr list(new typename ModelListPtr::element_type);
			
			BOOST_FOREACH(typename Multimap::left_const_reference& i, index.left.equal_range(boost::any_cast<const KeyType>(key)))
				list->push_back(i.second);
			
			return list;
		}
		
		virtual ModelClassPtr get(const boost::any& key) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			typename Multimap::left_const_iterator it = index.left.find(boost::any_cast<const KeyType>(key));
			if(it == index.left.end())
				throw InstanceNotFoundException(getClassName<typename ModelClassPtr::element_type>(), boost::lexical_cast<std::string>(boost::any_cast<const KeyType>(key)));
			return it->second;
		}
		
	protected:
		
		Multimap index;
};

#endif /* HASH_INDEX_H */
