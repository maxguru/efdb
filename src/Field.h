
#ifndef FIELD_H
#define FIELD_H

#include <list>
#include <typeinfo>
#include <stddef.h>
#include <boost/functional/hash.hpp>
#include <json/json.h>
#include <boost/smart_ptr.hpp>
#include <boost/type_traits/has_dereference.hpp>
#include <boost/mpl/if.hpp>
#include <gmpxx.h>

typedef std::size_t FieldId;

template <typename member_t, typename T>
constexpr std::size_t offset_of(member_t T::* p, T sample = T()) {
	return std::size_t(&(sample.*p)) - std::size_t(&sample);
}

template<typename MemberType, typename ClassType>
constexpr MemberType& getMemberType(MemberType ClassType::* p, ClassType* instance)
{
	return instance->*p;
}

template<typename MemberType, typename ModelClass>
constexpr ModelClass getModelClass(MemberType ModelClass::* p)
{
	return ModelClass();
}

template<typename ModelClassPtr, FieldId fieldId> std::size_t modelFieldOffset();
template<typename ModelClassPtr, FieldId fieldId> inline auto getModelFieldType();
template<typename ModelClassPtr, FieldId fieldId> inline auto& getModelFieldReference(ModelClassPtr& instance);
template<typename ModelClassPtr, FieldId fieldId> inline std::string getModelFieldName();

#define REGISTER_MODEL_FIELD(ModelClassPtr, fieldId, member) \
	template<> inline std::size_t modelFieldOffset<ModelClassPtr, fieldId>() { return offset_of(&member); } \
	template<> inline auto getModelFieldType<ModelClassPtr, fieldId>() { return decltype(member)(); } \
	template<> inline auto& getModelFieldReference<ModelClassPtr, fieldId>(ModelClassPtr& instance) { \
		return getMemberType(&member, static_cast<decltype(getModelClass(&member))*>(instance.get())); \
	} \
	template<> inline std::string getModelFieldName<ModelClassPtr, fieldId>() { \
		return #member; \
	} \

#define MODEL_FIELD_TYPE(ModelClassPtr, fieldId) decltype(getModelFieldType<ModelClassPtr, fieldId>())
#define MODEL_FIELD_REFERENCE(ModelClassPtr, fieldId, instance) getModelFieldReference<ModelClassPtr, fieldId>(instance)
#define MODEL_FIELD_NAME(ModelClassPtr, fieldId) getModelFieldName<ModelClassPtr, fieldId>()

class VoidParentModelType { };

template<typename ModelClass> inline auto getParentModelClass() { return VoidParentModelType(); }
#define REGISTER_PARENT_MODEL(ModelClass, ParentModelClass) template<> inline auto getParentModelClass<ModelClass>() { return ParentModelClass(); }
#define GET_PARENT_MODEL(ModelClass) decltype(getParentModelClass<ModelClass>())

class FieldBase;
typedef std::list<FieldBase*> FieldList;

template<typename ModelClassPtr, typename ModelClass>
void getFieldsHelper(ModelClassPtr& i, FieldList& l);

template<typename ModelClassPtr, typename ModelClass> inline void getModelFieldReferences(ModelClass& instance, FieldList& l)
{
	ModelClassPtr instPtr(boost::static_pointer_cast<typename ModelClassPtr::element_type>(instance.shared_from_this()));
	getFieldsHelper<ModelClassPtr, ModelClass>(instPtr, l);
}

#define REGISTER_MODEL_FIELD_IDS(ModelClassPtr, ModelClass, ...) \
	template<> inline void getModelFieldReferences<ModelClassPtr, ModelClass>(ModelClass& instance, FieldList& l) \
	{ \
		ModelClassPtr instPtr(boost::static_pointer_cast<typename ModelClassPtr::element_type>(instance.shared_from_this())); \
		getFieldsHelper<ModelClassPtr, ModelClass, __VA_ARGS__>(instPtr, l); \
	} \

template<typename ModelClassPtr, typename ModelClass> inline FieldList getModelFieldReferencesWrapper(ModelClass& instance)
{
	FieldList l;
	getModelFieldReferences<ModelClassPtr, ModelClass>(instance, l);
	return l;
}

template<typename ParentModelClass>
struct ParentFieldReferences
{
	template<typename ModelClassPtr>
	static inline void get(ModelClassPtr& i, FieldList& l)
	{
		getModelFieldReferences<ModelClassPtr, ParentModelClass>(*static_cast<ParentModelClass*>(i.get()), l);
	}
	
};
template<>
struct ParentFieldReferences<VoidParentModelType>
{
	template<typename ModelClassPtr>
	static inline void get(ModelClassPtr&, FieldList&) { }
};

template<typename ModelClassPtr, typename ModelClass>
inline void getFieldsHelper(ModelClassPtr& i, FieldList& l)
{
	typedef ParentFieldReferences<GET_PARENT_MODEL(ModelClass)> ForkType;
	ForkType::get(i, l);
}
template<typename ModelClassPtr, typename ModelClass, FieldId fieldId, FieldId... fieldIds>
inline void getFieldsHelper(ModelClassPtr& i, FieldList& l)
{
	l.push_back(&getModelFieldReference<ModelClassPtr, fieldId>(i));
	getFieldsHelper<ModelClassPtr, ModelClass, fieldIds...>(i, l);
}

inline Json::Value ValueToJsonValue(const std::size_t& value) { return Json::UInt64(value); };
inline Json::Value ValueToJsonValue(const Json::Int& value) { return value; };
inline Json::Value ValueToJsonValue(const Json::UInt& value) { return value; };
inline Json::Value ValueToJsonValue(const Json::Int64& value) { return value; };
inline Json::Value ValueToJsonValue(const double& value) { return value; };
inline Json::Value ValueToJsonValue(char* const& value) { return value; };
inline Json::Value ValueToJsonValue(const std::string& value) { return value; };
inline Json::Value ValueToJsonValue(const bool& value) { return value; };
inline Json::Value ValueToJsonValue(const mpz_class& value) { return value.get_str(); };
inline Json::Value ValueToJsonValue(const mpq_class& value) { return value.get_str(); };

class ModelBase;
inline Json::Value ValueToJsonValue(const ModelBase& value);

template<typename FieldType>
struct ValueFieldToJsonValue { inline Json::Value operator()(const FieldType& value) { return ValueToJsonValue(value); } };
template<typename FieldType>
struct PointerFieldToJsonValue { inline Json::Value operator()(const FieldType& value) { if(!value) return Json::Value::null; return ValueToJsonValue(*value); } };
template<typename FieldType>
struct FieldToJsonValue : public boost::mpl::if_<boost::has_dereference<FieldType>, PointerFieldToJsonValue<FieldType>, ValueFieldToJsonValue<FieldType> >::type { };

template<typename FieldType> inline FieldType JsonValueToValue(const Json::Value& value);

template<> inline std::size_t JsonValueToValue(const Json::Value& value) { return value.asUInt64(); };
template<> inline Json::Int JsonValueToValue(const Json::Value& value) { return value.asInt(); };
template<> inline Json::UInt JsonValueToValue(const Json::Value& value) { return value.asUInt(); };
template<> inline Json::Int64 JsonValueToValue(const Json::Value& value) { return value.asInt64(); };
template<> inline double JsonValueToValue(const Json::Value& value) { return value.asDouble(); };
template<> inline const char* JsonValueToValue(const Json::Value& value) { return value.asCString(); };
template<> inline std::string JsonValueToValue(const Json::Value& value) { return value.asString(); };
template<> inline bool JsonValueToValue(const Json::Value& value) { return value.asBool(); };
template<> inline mpz_class JsonValueToValue(const Json::Value& value) { return mpz_class(value.asString()); };
template<> inline mpq_class JsonValueToValue(const Json::Value& value) { return mpq_class(value.asString()); };

template<typename ModelClassPtr>
inline ModelClassPtr JsonValueToRelation(const Json::Value& value);

template<typename FieldType>
struct JsonValueToValueField { inline FieldType operator()(const Json::Value& value) { return JsonValueToValue<FieldType>(value); } };
template<typename FieldType>
struct JsonValueToPointerField { inline FieldType operator()(const Json::Value& value) { if(value.isNull()) return NULL; return JsonValueToValue<FieldType>(value); } };
template<typename FieldType>
struct JsonValueToFieldValue : public boost::mpl::if_<boost::has_dereference<FieldType>, JsonValueToPointerField<FieldType>, JsonValueToValueField<FieldType> >::type { };

namespace FieldPolicies
{
	enum FieldPolicy
	{
		None = 0,
		OnDeleteErase = 1 << 0,
		OnDeleteSetToNull = 1 << 1,
	};
}
inline FieldPolicies::FieldPolicy operator|(FieldPolicies::FieldPolicy a, FieldPolicies::FieldPolicy b)
{
	return static_cast<FieldPolicies::FieldPolicy>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

class FieldBase
{
	public:
		
		virtual ~FieldBase() { };
		
		virtual void* getParent(size_t offset) const
		{
			// dangerous code! can cause crashes!
			void* ptr =
				reinterpret_cast<void*>(
					const_cast<unsigned char*>(
						reinterpret_cast<const unsigned char*>(this)
					) - offset
				);
			
			return ptr;
		}
		virtual FieldId getFieldId() const = 0;
		virtual std::string getFieldName() const = 0;
		virtual Json::Value toJson() const = 0;
		virtual void fromJson(const Json::Value& value) = 0;
		virtual bool policyExists(FieldPolicies::FieldPolicy p) const = 0;
		
		// used by IndexedFields
		virtual void registerField() const { };
		virtual void updateIndex() const { };
		
		// used by RelationFields
		virtual void reset() { };
		virtual void modelDeleteHandler() { };
};

#include "Transaction.h"

template<typename T>
struct ModelStoreGetter;

template<typename FieldType, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies = FieldPolicies::None>
class Field : public FieldBase
{
	public:
		
		typedef FieldType type;
		static constexpr FieldId field_id = fieldId;
		
		Field() { }
		Field(const FieldType& v)
		: var(v)
		{ }
		
		virtual FieldId getFieldId() const
		{
			return fieldId;
		}
		
		virtual std::string getFieldName() const
		{
			return MODEL_FIELD_NAME(ModelClassPtr, fieldId);
		}
		
		virtual ModelClassPtr getModel() const
		{
			typedef typename ModelClassPtr::element_type ModelClass;
			return reinterpret_cast<ModelClass*>(this->getParent(modelFieldOffset<ModelClassPtr, fieldId>()))->mePtr();
		}
		
		virtual bool operator==(const Field<FieldType, fieldId, ModelClassPtr, fieldPolicies>& rhs) const
		{
			return var == rhs.var;
		}
		
		virtual bool operator==(const FieldType& rhs) const
		{
			return var == rhs;
		}
		
		//virtual const FieldType& operator*() const { return var; };
		//virtual const FieldType* operator->() const { return &var; };
		
		const auto& operator*() const { return *var; };
		const auto* operator->() const { return &(*var); };
		
		virtual const FieldType& get() const { return var; };
		
		virtual const FieldType& operator=(const FieldType& rhs)
		{
			// TODO: it may be a bit too expensive to lock on assignment, is it really necessary?
			TransactionPtr transaction = Transaction::startTransaction();
			transaction->getExclusiveLock(static_cast<Lockable*>(&ModelStoreGetter<ModelClassPtr>()()));
			
			var = rhs;
			return var;
		}
		
		virtual operator const FieldType&() const { return var; }
		
		virtual Json::Value toJson() const
		{
			return FieldToJsonValue<FieldType>()(var);
		}
		
		virtual void fromJson(const Json::Value& value)
		{
			var = JsonValueToFieldValue<FieldType>()(value);
		}
		
		virtual bool policyExists(FieldPolicies::FieldPolicy p) const
		{
			return (p & fieldPolicies) != 0;
		}
		
	protected:
		
		FieldType var;
};

template<typename FieldType, size_t fieldId, typename ModelClassPtr, FieldPolicies::FieldPolicy fieldPolicies = FieldPolicies::None>
size_t hash_value(const Field<FieldType, fieldId, ModelClassPtr, fieldPolicies>& i)
{
	return boost::hash<FieldType>()(i.get());
}

#include "Model.h"

#endif /* FIELD_H */
