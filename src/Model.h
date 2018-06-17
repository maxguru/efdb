
#ifndef MODEL_H
#define	MODEL_H

#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <json/value.h>

template<typename T>
inline std::string getClassName()
{
	std::string name = typeid(T).name();
	#ifdef __GNUC__
		int status;
		char *realname = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
		name = realname;
		free(realname);
	#endif
	
	return name;
}

template<typename T>
inline std::string getClassName(const T& t)
{
	std::string name = typeid(t).name();
	#ifdef __GNUC__
		int status;
		char *realname = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
		name = realname;
		free(realname);
	#endif
	
	return name;
}

typedef std::size_t ModelId;

class ModelBase : public boost::enable_shared_from_this<ModelBase>
{
	public:
		virtual ModelId getId() const = 0;
		virtual std::string getModelName() const = 0;
};

template<typename ModelClassPtr>
class Model;

typedef boost::shared_ptr<Json::Value const> JsonValueConstPtr;
typedef boost::shared_ptr<Json::Value> JsonValuePtr;

#define MODEL_FRIENDS \
	template<typename ModelClassPtr> friend class ::Model; \
	template<typename ModelClassPtr> friend class ::ModelStore; \
	template<typename ModelClassPtr_, FieldId fieldId_> friend std::size_t modelFieldOffset(); \
	template<typename member_t, typename T> friend constexpr std::size_t offset_of(member_t T::* p, T sample); \
	template<typename MemberType, typename ClassType> friend constexpr MemberType& getMemberType(MemberType ClassType::* p, ClassType* instance); \
	template<typename MemberType, typename ModelClass> friend constexpr ModelClass getModelClass(MemberType ModelClass::* p); \
	template<typename ModelClassPtr, FieldId fieldId> friend auto getModelFieldType(); \
	template<typename ModelClassPtr, FieldId id> friend auto& getModelFieldReference(ModelClassPtr& instance); \
	template<typename ModelClass> friend void getModelFieldReferences(ModelClass& instance, FieldList& l); \
	template<typename ModelClass> friend auto getParentModelClass(); \
	template<typename ModelClassPtr, typename ModelClass> friend class ModelContainer; \

#define DO_AUTOMATIC_CLEANUP virtual bool isAutomaticCleanupEnabled() { return true; }

#include "Transaction.h"
#include "ModelStore.h"
#include "IndexedField.h"
#include "RelationField.h"

inline Json::Value ValueToJsonValue(const ModelBase& value) { return Json::UInt64(value.getId()); };
template<typename ModelClassPtr>
inline ModelClassPtr JsonValueToValue(const Json::Value& value) { return ModelStoreGetter<ModelClassPtr>()().getInstance(value.asUInt64()); };

template<typename ModelClassPtr>
class Model : public ModelBase
{
	public:
		
		typedef typename ModelClassPtr::element_type DerrivedClass;
		
		virtual ~Model() { };
		
		virtual ModelId getId() const
		{
			return getModelStore().getId(this->mePtr());
		}
		
		virtual ModelStore<ModelClassPtr>& getModelStore() const
		{
			return ModelStoreGetter<ModelClassPtr>()();
		}
		
		virtual ModelId store()
		{
			TransactionPtr transaction = Transaction::startTransaction();
			ModelStore<ModelClassPtr>& store(getModelStore());
			ModelId id = store.store(this->mePtr());
			store.updateIndexes(this->mePtr());
			return id;
		}
		
		virtual void erase() { getModelStore().erase(this->mePtr()); }
		virtual bool hasReferences() { return getModelStore().hasRelationReferences(this->mePtr()); }
		
		virtual bool isAutomaticCleanupEnabled() { return false; }
		virtual void doAutomaticCleanup()
		{
			try { this->getId(); }
			catch(const InstanceNotFoundException&) { return; }
			
			if(this->isAutomaticCleanupEnabled() && !this->hasReferences())
				this->erase();
		}
		
		virtual std::string getModelName() const { return getClassName(*this); }
		virtual JsonValuePtr toJson() { return getModelStore().toJson(this->mePtr()); }
		
		virtual ModelClassPtr mePtr() const
		{
			return boost::const_pointer_cast<DerrivedClass>(
					boost::static_pointer_cast<const DerrivedClass>(
						this->shared_from_this()
					)
				);
		}
		
	protected:
		
		Model() { }
		
	private:
		
};

#endif	/* MODEL_H */
