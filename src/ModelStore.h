
#ifndef MODEL_STORE_H
#define MODEL_STORE_H

#include <vector>
#include <set>
#include <boost/smart_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/container/map.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <json/json.h>
#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>
#include <gmpxx.h>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

class ModelStoreBase;

template<typename ModelClassPtr>
class ModelStore;

#include "KeyOperators.h"
#include "Transaction.h"
#include "Index.h"
#include "HashIndex.h"
#include "CompoundIndex.h"
#include "RelationStore.h"
#include "InstanceNotFoundException.h"
#include "Field.h"

template<typename ModelClassPtr>
class ModelContainerBase
{
	public:
		virtual bool matchType(ModelClassPtr instance) const = 0;
		virtual FieldList getModelInstanceFields(ModelClassPtr instance) const = 0;
		virtual ModelClassPtr construct() const = 0;
		virtual std::string getModelName() const = 0;
};

template<typename ModelClassPtr, typename ModelClass>
class ModelContainer : public ModelContainerBase<ModelClassPtr>
{
	public:
		
		virtual inline bool matchType(ModelClassPtr instance) const
		{
			return typeid(ModelClass) == typeid(*instance);
		}
		virtual inline FieldList getModelInstanceFields(ModelClassPtr instance) const
		{
			return getModelFieldReferencesWrapper<ModelClassPtr, ModelClass>(*static_cast<ModelClass*>(instance.get()));
		}
		virtual inline ModelClassPtr construct() const
		{
			return ModelClassPtr(new ModelClass);
		}
		virtual std::string getModelName() const
		{
			return getClassName<ModelClass>();
		}
};

class ModelStoreBase : public Lockable
{
	public:
		
		virtual void triggerRelationModelDeleteEvent(const boost::any& instance) = 0;
		virtual bool hasIndexReferences(const boost::any& instance) const = 0;
};

template<typename ModelClassPtr>
class ModelStore : public ModelStoreBase
{
	public:
		
		typedef size_t ID;
		typedef ModelClassPtr ValueType;
		
		typedef ModelContainerBase<ModelClassPtr> ModelContainerType;
		typedef boost::shared_ptr< ModelContainerType > ModelContainerPtr;
		typedef boost::container::map<std::string, ModelContainerPtr> ModelClasses;
		
		typedef boost::bimap<
			boost::bimaps::unordered_set_of<ID>,
			boost::bimaps::unordered_set_of< ModelClassPtr, value_key_operators::hash<ModelClassPtr>, value_key_operators::equality<ModelClassPtr> >
		> Multimap;
		
		typedef typename Multimap::value_type IndexElementType;
		
		typedef boost::shared_ptr< std::vector<ModelClassPtr> > ModelListPtr;
		typedef boost::shared_ptr< std::vector<ID> > IDList;
		
		typedef boost::shared_ptr< Index<ModelClassPtr> > IndexPtr;
		typedef std::set<FieldId> FieldSet;
		typedef boost::unordered_map<FieldSet, IndexPtr> Indexes;
		
		typedef RelationStoreBase<ModelClassPtr> RelationStore;
		typedef std::set<RelationStore*> Relations;
		
		typedef std::set<ModelStoreBase*> RelationModels;
		
		ModelStore() { };
		virtual ~ModelStore() { };
		
		template<typename ModelClass>
		void registerModel()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			ModelContainerPtr model(new ModelContainer<ModelClassPtr, ModelClass>());
			models[model->getModelName()] = model;
			registerFields(model);
		}
		
		template<typename ModelClass>
		void unregisterModel()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			models.erase(ModelContainer<ModelClassPtr, ModelClass>()->getModelName());
		}
		
		inline FieldList getModelInstanceFields(ModelClassPtr instance) const
		{
			BOOST_FOREACH(typename ModelClasses::value_type it, models)
			{
				ModelContainerPtr model(it.second);
				if(model->matchType(instance))
					return model->getModelInstanceFields(instance);
			}
			throw std::runtime_error("Model type not registered");
		}
		
		inline ModelContainerPtr getModelContainer(ModelClassPtr instance)
		{
			BOOST_FOREACH(typename ModelClasses::value_type it, models)
			{
				ModelContainerPtr model(it.second);
				if(model->matchType(instance))
					return model;
			}
			throw std::runtime_error("Model type not registered");
		}
		
		virtual void registerRelationStore(RelationStore* relation)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			relations.insert(relation);
		}
		
		virtual void unregisterRelationStore(RelationStore* relation)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			relations.erase(relation);
		}
		
		virtual void registerRelationModelStore(ModelStoreBase* modelStore)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			relationModels.insert(modelStore);
		}
		
		virtual void unregisterRelationModelStore(ModelStoreBase* modelStore)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			relationModels.erase(modelStore);
		}
		
		virtual ID store(ModelClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			typename Multimap::right_const_iterator it = instances.right.find(instance);
			if(it != instances.right.end())
				return it->second;
			ID id = generateId(instance);
			instances.insert(IndexElementType(id, instance));
			return id;
		}
		
		virtual ID getId(ModelClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			typename Multimap::right_const_iterator it = instances.right.find(instance);
			if(it == instances.right.end())
				throw InstanceNotFoundException(instance->getModelName(), boost::lexical_cast<std::string>(instance));
			return it->second;
		}
		
		virtual const ModelClassPtr getInstance(ID id) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			typename Multimap::left_const_iterator it = instances.left.find(id);
			if(it == instances.left.end())
				throw InstanceNotFoundException(getClassName<typename ModelClassPtr::element_type>(), boost::lexical_cast<std::string>(id));
			return it->second;
		}
		
		virtual void erase(ID id)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			typename Multimap::left_const_iterator it = instances.left.find(id);
			if(it == instances.left.end())
				return;
			
			eraseHelper(it->second);
		}
		
		virtual void erase(ModelClassPtr instance)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			eraseHelper(instance);
		}
		
		virtual ModelListPtr getList() const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			ModelListPtr list(new typename ModelListPtr::element_type);
			list->reserve(instances.left.size());
			BOOST_FOREACH(typename Multimap::left_const_reference& i, instances.left)
				list->push_back(i.second);
			return list;
		}
		
		virtual std::size_t getCount() const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			return std::distance(instances.left.begin(),instances.left.end());
		}
		
		virtual IDList getIdList() const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			IDList list(new typename IDList::element_type);
			list->reserve(instances.left.size());
			BOOST_FOREACH(typename Multimap::left_const_reference& i, instances.left)
				list->push_back(i.first);
			return list;
		}
		
		template <FieldId fieldId, typename FieldType>
		ModelListPtr getList(const FieldType & value)
		{
			return getIndex(fieldId)->getList(static_cast<typename MODEL_FIELD_TYPE(ModelClassPtr, fieldId)::type>(value));
		}
		
		template <typename... Args>
		inline ModelListPtr getListHelper(IndexPtr index, Args... args)
		{
			return index->getList(boost::make_tuple(args...));
		}
		
		template <FieldId fieldId, FieldId... fieldIds, typename FieldType, typename... Args>
		inline ModelListPtr getListHelper(IndexPtr index, const FieldType & value, Args... args)
		{
			return getListHelper<fieldIds...>(index, args..., static_cast<typename MODEL_FIELD_TYPE(ModelClassPtr, fieldId)::type>(value));
		}
		
		template <FieldId fieldId1, FieldId fieldId2, FieldId... fieldIds, typename... Args>
		ModelListPtr getList(Args... args)
		{
			IndexPtr index = getIndex(fieldId1, fieldId2, fieldIds...);
			return getListHelper<fieldId1, fieldId2, fieldIds...>(index, args...);
		}
		
		template <FieldId fieldId, typename FieldType>
		ModelClassPtr get(const FieldType & value)
		{
			return getIndex(fieldId)->get(static_cast<typename MODEL_FIELD_TYPE(ModelClassPtr, fieldId)::type>(value));
		}
		
		template <typename... Args>
		inline ModelClassPtr getHelper(IndexPtr index, Args... args)
		{
			return index->get(boost::make_tuple(args...));
		}
		
		template <FieldId fieldId, FieldId... fieldIds, typename FieldType, typename... Args>
		inline ModelClassPtr getHelper(IndexPtr index, const FieldType & value, Args... args)
		{
			return getHelper<fieldIds...>(index, args..., static_cast<typename MODEL_FIELD_TYPE(ModelClassPtr, fieldId)::type>(value));
		}
		
		template <FieldId fieldId1, FieldId fieldId2, FieldId... fieldIds, typename... Args>
		ModelClassPtr get(Args... args)
		{
			IndexPtr index = getIndex(fieldId1, fieldId2, fieldIds...);
			return getHelper<fieldId1, fieldId2, fieldIds...>(index, args...);
		}
		
		virtual void registerFields(ModelContainerPtr model)
		{
			// TODO: remove construct() call
			ModelClassPtr instance(model->construct());
			BOOST_FOREACH(const FieldBase* field, model->getModelInstanceFields(instance))
			{
				field->registerField();
			}
		}
		
		virtual void updateIndexes(ModelClassPtr model)
		{
			BOOST_FOREACH(FieldBase* field, getModelInstanceFields(model))
			{
				field->updateIndex();
			}
		}
		
		virtual JsonValuePtr toJson(ModelClassPtr model) const
		{
			JsonValuePtr json(new Json::Value);
			
			BOOST_FOREACH(const FieldBase* field, getModelInstanceFields(model))
			{
				(*json)[field->getFieldName()] = field->toJson();
			}
			
			return json;
		}
		
		virtual ModelClassPtr fromJson(ModelContainerPtr model, const Json::Value& values) const
		{
			ModelClassPtr instance(model->construct());
			
			BOOST_FOREACH(FieldBase* field, model->getModelInstanceFields(instance))
			{
				field->fromJson(values[field->getFieldName()]);
			}
			
			return instance;
		}
		
		inline void fieldSetAppender(FieldSet& fields, FieldId fieldId)
		{
			fields.insert(fieldId);
		}
		template<typename... FieldIds>
		inline void fieldSetAppender(FieldSet& fields, FieldId fieldId, FieldIds... fieldIds)
		{
			fieldSetAppender(fields, fieldId);
			fieldSetAppender(fields, fieldIds...);
		}
		
		template<typename... FieldIds>
		void addIndex(IndexPtr index, FieldIds... fieldIds)
		{
			FieldSet fields;
			fieldSetAppender(fields, fieldIds...);
			
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			indexes[fields] = index;
		}
		
		template<typename FieldType>
		inline void addIndex()
		{
			addIndex(boost::shared_ptr< Index<ModelClassPtr> >(new HashIndex<ModelClassPtr, typename FieldType::type>), FieldType::field_id);
		}
		
		template<FieldId fieldId>
		inline void addIndex()
		{
			addIndex(boost::shared_ptr< Index<ModelClassPtr> >(new HashIndex<ModelClassPtr, typename MODEL_FIELD_TYPE(ModelClassPtr, fieldId)::type>), fieldId);
		}
		
		template<typename... FieldTypes>
		inline void addCompoundIndex()
		{
			addIndex(boost::shared_ptr< Index<ModelClassPtr> >(new CompoundIndex<ModelClassPtr, FieldTypes...>), FieldTypes::field_id...);
		}
		
		template<FieldId... fieldIds>
		inline void addCompoundIndex()
		{
			addIndex(boost::shared_ptr< Index<ModelClassPtr> >(new CompoundIndex<ModelClassPtr, MODEL_FIELD_TYPE(ModelClassPtr, fieldIds)...>), fieldIds...);
		}
		
		template<typename... FieldIds>
		IndexPtr getIndex(FieldIds... fieldIds)
		{
			FieldSet fields;
			fieldSetAppender(fields, fieldIds...);
			
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			typename Indexes::const_iterator it = indexes.find(fields);
			if(it == indexes.end())
				// TODO: fix
				throw std::runtime_error("Failed to find index");
			
			return it->second;
		}
		
		template <typename FieldType>
		void updateIndex(ModelClassPtr instance, FieldId fieldId, const FieldType & value)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			typename Multimap::right_const_iterator iit = instances.right.find(instance);
			if(iit == instances.right.end())
				return;
			
			BOOST_FOREACH(typename Indexes::value_type i, indexes)
			{
				const FieldSet& fieldSet(i.first);
				if(fieldSet.find(fieldId) != fieldSet.end())
				{
					IndexPtr index(i.second);
					index->store(fieldId, value, instance);
				}
			}
		}
		
		virtual bool hasIndexReferences(const boost::any& instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			// go through relation indexes and check if they contain instance
			BOOST_FOREACH(typename Indexes::value_type i, indexes)
			{
				IndexPtr index(i.second);
				
				if(!index->isRelationIndex())
					continue;
				
				// trying to match relation index pointing to instance
				if(!index->matchKeyType(instance))
					continue;
				
				try
				{
					ModelClassPtr referencingInstance = index->get(instance);
					if(referencingInstance)
						if(!referencingInstance->isAutomaticCleanupEnabled())
							return true;
				}
				catch(const InstanceNotFoundException&) { }
			}
			
			return false;
		}
		
		virtual bool hasRelationReferences(ModelClassPtr instance) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			BOOST_FOREACH(typename RelationModels::value_type i, relationModels)
			{
				if(i->hasIndexReferences(instance))
					return true;
			}
			
			BOOST_FOREACH(typename Relations::value_type i, relations)
			{
				if(i->exists(instance))
					return true;
			}
			
			return false;
		}
		
		virtual void doGarbageCollection()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			for(auto it = instances.begin(); it != instances.end();)
			{
				ModelClassPtr instance(it->right);
				if(instance->isAutomaticCleanupEnabled() && !instance->hasReferences())
				{
					it = instances.erase(it);
					instance->erase();
					continue;
				}
				++it;
			}
		}
		
		virtual FieldPolicies::FieldPolicy getFieldDeletePolicy(FieldId fieldId)
		{
			// for each registered models
			BOOST_FOREACH(typename ModelClasses::value_type it, models)
			{
				ModelContainerPtr model(it.second);
				
				// TODO: remove construct() call
				ModelClassPtr instance = model->construct();
				BOOST_FOREACH(const FieldBase* field, model->getModelInstanceFields(instance))
				{
					if(field->getFieldId() == fieldId)
					{
						if(field->policyExists(FieldPolicies::OnDeleteErase))
							return FieldPolicies::OnDeleteErase;
						if(field->policyExists(FieldPolicies::OnDeleteSetToNull))
							return FieldPolicies::OnDeleteSetToNull;
						return FieldPolicies::None;
					}
				}
			}
			
			throw std::runtime_error("Field not found");
		}
		
		virtual void triggerRelationModelDeleteEvent(const boost::any& deletingInstance)
		{
			// go through relation indexes and trigger delete event on each instance that points to deletingInstance
			
			BOOST_FOREACH(typename Indexes::value_type ind, indexes)
			{
				IndexPtr index = ind.second;
				
				if(!index->isRelationIndex())
					continue;
				
				// trying to match relation index pointing to deletingInstance
				if(!index->matchKeyType(deletingInstance))
					continue;
				
				const FieldSet& fieldIds = ind.first;
				
				// ignore compound indexes
				if(fieldIds.size() != 1)
					continue;
				
				FieldId fieldId = *(fieldIds.begin());
				
				FieldPolicies::FieldPolicy deletePolicy(getFieldDeletePolicy(fieldId));
				
				ModelListPtr list = index->getList(deletingInstance);
				BOOST_FOREACH(ModelClassPtr& instance, *list)
				{
					switch(deletePolicy)
					{
						case FieldPolicies::None:
							// erase if no flag is set
						case FieldPolicies::OnDeleteErase:
							instance->erase();
							break;
						case FieldPolicies::OnDeleteSetToNull:
							BOOST_FOREACH(FieldBase* field, getModelInstanceFields(instance))
							{
								if(field->getFieldId() == fieldId)
								{
									field->reset();
									break;
								}
							}
					}
				}
			}
		}
		
		virtual void clear()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			BOOST_FOREACH(typename Multimap::left_const_reference& i, instances.left)
				BOOST_FOREACH(typename RelationModels::value_type r, relationModels)
					r->triggerRelationModelDeleteEvent(i.second);
			
			instances.clear();
			BOOST_FOREACH(typename Indexes::value_type i, indexes)
				i.second->clear();
			BOOST_FOREACH(typename Relations::value_type i, relations)
				i->clear();
		}
		
		virtual std::size_t size() const
		{
			return instances.left.size();
		}
		
		virtual void exportJson(const std::string filepath, double* progress = NULL, double* total = NULL) const
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getSharedLock(this);
			
			Json::StreamWriterBuilder builder;
			builder["indentation"] = "\t";
			boost::scoped_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
			
			std::ofstream outfile(filepath.c_str(), std::ofstream::binary);
			
			boost::posix_time::ptime printTime = boost::posix_time::second_clock::local_time();
			
			BOOST_FOREACH(typename Multimap::left_const_reference& i, instances.left)
			{
				Json::Value value;
				value["id"] = Json::UInt64(i.first);
				value["model"] = i.second->getModelName();
				value["fields"] = *(toJson(i.second));
				writer->write(value, &outfile);
				outfile << std::endl;
				
				if(progress != NULL && total != NULL)
				{
					*progress += 1.0;
					boost::posix_time::ptime currentTime = boost::posix_time::second_clock::local_time();
					if(printTime + boost::posix_time::seconds(1) < currentTime)
					{
						double percent = (*progress / *total) * 100.0;
						printTime = currentTime;
						std::cout << "[" << percent << "%] Exporting to " << filepath << std::endl;
					}
				}
			}
		}
		
		void importJson(const std::string filepath, double* progress = NULL, double* total = NULL)
		{
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(this);
			
			std::ifstream infile(filepath.c_str(), std::ifstream::binary);
			
			boost::posix_time::ptime printTime = boost::posix_time::second_clock::local_time();
			double oldpos = 0;
			
			Json::CharReaderBuilder rbuilder;
			rbuilder["collectComments"] = false;
			rbuilder["strictRoot"] = true;
			rbuilder["rejectDupKeys"] = true;
			rbuilder["failIfExtra"] = true;
			rbuilder["stackLimit"] = 10000;
			
			std::stringstream data;
			while(infile.peek() != EOF)
			{
				if(progress != NULL && total != NULL)
				{
					double curpos = infile.tellg();
					*progress += curpos - oldpos;
					oldpos = curpos;
					boost::posix_time::ptime currentTime = boost::posix_time::second_clock::local_time();
					if(printTime + boost::posix_time::seconds(1) < currentTime)
					{
						double percent = (*progress / *total) * 100.0;
						printTime = currentTime;
						std::cout << "[" << percent << "%] Importing from " << filepath << std::endl;
					}
				}
				
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
				
				ID id = root["id"].asUInt64();
				std::string modelName = root["model"].asString();
				
				ModelContainerPtr model;
				{
					typename ModelClasses::const_iterator it = models.find(modelName);
					if(it == models.end())
						throw std::runtime_error("Invalid model '" + modelName + "'");
					model = it->second;
				}
				
				ModelClassPtr instance = fromJson(model, root["fields"]);
				
				typename Multimap::left_const_iterator idit = instances.left.find(id);
				typename Multimap::right_const_iterator init = instances.right.find(instance);
				if(init != instances.right.end() || idit != instances.left.end())
					throw std::runtime_error("Model " + boost::lexical_cast<std::string>(id) + " already exists");
				instances.insert(IndexElementType(id, instance));
				updateIndexes(instance);
			}
			
			infile.close();
		}
		
	private:
		
		ID generateId(ModelClassPtr instance) const
		{
			ID id = key_hash(instance);
			
			while(true)
			{
				typename Multimap::left_const_iterator it = instances.left.find(id);
				if(it == instances.left.end())
					return id;
				id++;
			}
		}
		
		virtual void eraseHelper(ModelClassPtr instance)
		{
			BOOST_FOREACH(typename RelationModels::value_type i, relationModels)
				i->triggerRelationModelDeleteEvent(instance);
			
			instances.right.erase(instance);
			BOOST_FOREACH(typename Indexes::value_type i, indexes)
				i.second->erase(instance);
			BOOST_FOREACH(typename Relations::value_type i, relations)
				i->erase(instance);
			
			BOOST_ASSERT(hasRelationReferences(instance)==false);
			
			// TODO: notify relation fields of deletion
			BOOST_FOREACH(FieldBase* field, getModelInstanceFields(instance))
			{
				field->modelDeleteHandler();
			}
		}
		
		ModelClasses models;
		Multimap instances;
		Indexes indexes;
		Relations relations;
		RelationModels relationModels;
};

template<typename T>
struct ModelStoreGetter
{
	ModelStore<T>& operator()() const;
};

#endif /* MODEL_STORE_H */
