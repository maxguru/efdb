
#ifndef RELATION_STORE_H
#define RELATION_STORE_H

#include <algorithm>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <json/json.h>
#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

template<typename ModelClassPtr>
class RelationStoreBase
{
	public:
		RelationStoreBase() { };
		virtual ~RelationStoreBase() { };
		
		virtual bool exists(ModelClassPtr instance) const = 0;
		virtual void erase(ModelClassPtr instance) = 0;
		virtual void clear() = 0;
};

template<typename ModelAClassPtr, typename ModelBClassPtr>
class RelationStore;

#include "InstanceNotFoundException.h"
#include "KeyOperators.h"
#include "Lockable.h"
#include "Model.h"

template<typename ModelAClassPtr, typename ModelBClassPtr>
class RelationStore : public RelationStoreBase<ModelAClassPtr>, public RelationStoreBase<ModelBClassPtr>, public Lockable
{
	public:
		
		typedef boost::bimap<
			boost::bimaps::unordered_multiset_of< ModelAClassPtr, value_key_operators::hash<ModelAClassPtr>, value_key_operators::equality<ModelAClassPtr> >,
			boost::bimaps::unordered_multiset_of< ModelBClassPtr, value_key_operators::hash<ModelBClassPtr>, value_key_operators::equality<ModelBClassPtr> >,
			boost::bimaps::unordered_set_of_relation< value_key_operators::relation_hash<boost::bimaps::_relation>, value_key_operators::relation_equality<boost::bimaps::_relation> >
		> Multimap;
		typedef typename Multimap::value_type IndexElementType;
		
		typedef boost::shared_ptr< std::vector<ModelAClassPtr> > ModelAListPtr;
		typedef boost::shared_ptr< std::vector<ModelBClassPtr> > ModelBListPtr;
		
		enum EraseModelPolicy {None, EraseModelWhenErasingRelation};
		
		RelationStore(ModelStore<ModelAClassPtr>& a, EraseModelPolicy aepol, ModelStore<ModelBClassPtr>& b, EraseModelPolicy bepol)
		: aModelStore(a), bModelStore(b), aErasePolicy(aepol), bErasePolicy(bepol)
		{
			a.registerRelationStore(this);
			b.registerRelationStore(this);
		};
		
		virtual ~RelationStore()
		{
			aModelStore.unregisterRelationStore(this);
			bModelStore.unregisterRelationStore(this);
		}
		
		virtual void store(ModelAClassPtr instanceA, ModelBClassPtr instanceB)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			typename Multimap::const_iterator it = relations.find(IndexElementType(instanceA, instanceB));
			if(it == relations.end())
				relations.insert(IndexElementType(instanceA, instanceB));
		}
		
		virtual void erase(ModelAClassPtr instanceA, ModelBClassPtr instanceB)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			relations.erase(IndexElementType(instanceA, instanceB));
			
			eraseAModel(instanceA);
			eraseBModel(instanceB);
		}
		
		virtual void erase(ModelAClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			if(bErasePolicy == None)
			{
				std::pair<typename Multimap::left_iterator, typename Multimap::left_iterator> range = relations.left.equal_range(instance);
				relations.left.erase(range.first, range.second);
			}
			else if(bErasePolicy == EraseModelWhenErasingRelation)
			{
				typename Multimap::left_iterator i;
				while((i = relations.left.find(instance)) != relations.left.end())
				{
					ModelBClassPtr bInstance = i->second;
					relations.left.erase(i);
					eraseBModel(bInstance);
				}
			}
			
			eraseAModel(instance);
		}
		
		virtual void erase(ModelBClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			std::pair<typename Multimap::right_iterator, typename Multimap::right_iterator> range = relations.right.equal_range(instance);
			
			if(aErasePolicy == None)
				relations.right.erase(range.first, range.second);
			else if(aErasePolicy == EraseModelWhenErasingRelation)
			{
				typename Multimap::right_iterator i;
				while((i = relations.right.find(instance)) != relations.right.end())
				{
					ModelAClassPtr aInstance = i->second;
					relations.right.erase(i);
					eraseAModel(aInstance);
				}
			}
			
			eraseBModel(instance);
		}
		
		virtual void clear()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			if(aErasePolicy == None && bErasePolicy == None)
				relations.clear();
			else
			{
				typename Multimap::iterator i;
				while((i = relations.begin()) != relations.end())
				{
					ModelAClassPtr aInstance = i->left;
					ModelBClassPtr bInstance = i->right;
					relations.erase(i);
					eraseAModel(aInstance);
					eraseBModel(bInstance);
				}
			}
		}
		
		virtual bool exists(ModelAClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			return relations.left.find(instance) != relations.left.end();
		}
		
		virtual bool exists(ModelBClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			return relations.right.find(instance) != relations.right.end();
		}
		
		virtual ModelBListPtr getList(ModelAClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			std::pair<typename Multimap::left_const_iterator, typename Multimap::left_const_iterator> range = relations.left.equal_range(instance);
			ModelBListPtr list(new typename ModelBListPtr::element_type);
			list->reserve(std::distance(range.first, range.second));
			BOOST_FOREACH(typename Multimap::left_const_reference& i, range)
				list->push_back(i.second);
			
			return list;
		}
		
		virtual ModelAListPtr getList(ModelBClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			std::pair<typename Multimap::right_const_iterator, typename Multimap::right_const_iterator> range = relations.right.equal_range(instance);
			ModelAListPtr list(new typename ModelAListPtr::element_type);
			list->reserve(std::distance(range.first, range.second));
			BOOST_FOREACH(typename Multimap::right_const_reference& i, range)
				list->push_back(i.second);
			return list;
		}
		
		virtual void exportJson(const std::string filepath) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			Json::StreamWriterBuilder builder;
			builder["indentation"] = "\t";
			boost::scoped_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
			
			std::ofstream outfile(filepath.c_str(), std::ofstream::binary);
			
			BOOST_FOREACH(const IndexElementType& i, relations)
			{
				Json::Value value;
				value["A"] = Json::UInt64(i.left->getId());
				value["B"] = Json::UInt64(i.right->getId());
				writer->write(value, &outfile);
				outfile << std::endl;
			}
		}
		
		void importJson(const std::string filepath)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			std::ifstream infile(filepath.c_str(), std::ifstream::binary);
			
			Json::CharReaderBuilder rbuilder;
			rbuilder["collectComments"] = false;
			rbuilder["strictRoot"] = true;
			rbuilder["rejectDupKeys"] = true;
			rbuilder["failIfExtra"] = true;
			
			std::stringstream data;
			while(infile.peek() != EOF)
			{
				// TODO: fix parseFromStream so that it reads one object at a time using a better method
				std::string dat;
				do
				{
					getline(infile, dat, '\n');
					data << dat << std::endl;
				} while(dat!="" && dat!="}");
				data.seekg(0);
				
				Json::Value root;
				std::string errs;
				bool ok = Json::parseFromStream(rbuilder, data, &root, &errs);
				if(!ok)
					continue;
				
				data.str("");
				
				ModelId aId = root["A"].asUInt64();
				ModelId bId = root["B"].asUInt64();
				
				try
				{
					ModelAClassPtr aInstance = aModelStore.getInstance(aId);
					ModelBClassPtr bInstance = bModelStore.getInstance(bId);
					store(aInstance, bInstance);
				}
				catch(const InstanceNotFoundException& e)
				{
					// TODO: fix
					std::cout << "instance not found" << std::endl;
				}
				
			}
			
			infile.close();
		}
		
	private:
		
		void eraseAModel(ModelAClassPtr instance)
		{
			if(aErasePolicy == EraseModelWhenErasingRelation)
				if(relations.left.find(instance) == relations.left.end())
					aModelStore.erase(instance);
			
			instance->doAutomaticCleanup();
		}
		
		void eraseBModel(ModelBClassPtr instance)
		{
			if(bErasePolicy == EraseModelWhenErasingRelation)
				if(relations.right.find(instance) == relations.right.end())
					bModelStore.erase(instance);
			
			instance->doAutomaticCleanup();
		}
		
		// disable copy
		RelationStore(const RelationStore&) { };
		const RelationStore& operator=(const RelationStore&) { };
		
		ModelStore<ModelAClassPtr>& aModelStore;
		ModelStore<ModelBClassPtr>& bModelStore;
		
		Multimap relations;
		EraseModelPolicy aErasePolicy;
		EraseModelPolicy bErasePolicy;
};

#endif /* MODEL_STORE_H */
