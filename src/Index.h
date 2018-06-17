
#ifndef INDEX_H
#define INDEX_H

#include <boost/any.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

template<typename ModelClassPtr>
class Index;

#include "Field.h"

template<typename ModelClassPtr>
class Index
{
	public:
		
		typedef typename ModelStore<ModelClassPtr>::ModelListPtr ModelListPtr;
		
		Index() { };
		virtual ~Index() { };
		
		virtual bool matchKeyType(const boost::any& key) = 0;
		
		virtual void store(FieldId id, const boost::any& key, ModelClassPtr instance) = 0;
		virtual void erase(ModelClassPtr instance) = 0;
		
		virtual void clear() = 0;
		
		virtual ModelListPtr getList(const boost::any& key) const = 0;
		virtual ModelClassPtr get(const boost::any& key) const = 0;
		
		virtual bool isRelationIndex() const { return false; }
		virtual bool isCompoundIndex() const { return false; }
		
		// only used with compound indexes
		template<typename... Fields>
		ModelListPtr getList(typename Fields::type... values)
		{
			return this->getList(boost::tuple<typename Fields::type...>(values...));
		}
		template<typename... Fields>
		ModelClassPtr get(typename Fields::type... values)
		{
			return this->get(boost::tuple<typename Fields::type...>(values...));
		}
};

#endif /* INDEX_H */
