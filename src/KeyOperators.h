
#ifndef KEY_OPERATORS_H
#define KEY_OPERATORS_H

#include <boost/type_traits/has_dereference.hpp>
#include <boost/mpl/if.hpp>

template<typename ValueType>
struct value_hash_combine { inline void operator()(std::size_t& seed, const ValueType& value) { boost::hash_combine(seed, value); } };
template<typename ValueType>
struct pointer_hash_combine { inline void operator()(std::size_t& seed, const ValueType& value) { if(value==NULL) boost::hash_combine(seed, 0); else boost::hash_combine(seed, *value); } };
template<typename ValueType>
struct smart_hash_combine : public boost::mpl::if_<boost::has_dereference<ValueType>, pointer_hash_combine<ValueType>, value_hash_combine<ValueType> >::type { };

template<typename key_type>
size_t key_hash(const key_type & key)
{
	return boost::hash<key_type>()(key);
}

namespace pointer_key_operators
{
	template<typename key_type>
	struct hash
	{
		public:
			size_t operator()(const key_type ptr) const
			{
				if(!ptr)
					return 0;
				return key_hash(*ptr);
			}
	};
	
	template<typename key_type>
	struct equality
	{
		public:
			bool operator()(const key_type& ptr1, const key_type& ptr2) const
			{
				if(!ptr1)
					return !ptr2;
				if(!ptr2)
					return false;
				return *ptr1 == *ptr2;
			}
	};
	
	template<typename relation_type>
	struct relation_hash
	{
		public:
			size_t operator()(const relation_type& rel) const
			{
				size_t seed = 0;
				if(rel.left)
					boost::hash_combine(seed, *(rel.left));
				else
					boost::hash_combine(seed, 0);
				if(rel.right)
					boost::hash_combine(seed, *(rel.right));
				else
					boost::hash_combine(seed, 0);
				return seed;
			}
	};
	
	template<typename relation_type>
	struct relation_equality
	{
		public:
			bool operator()(const relation_type& rel1, const relation_type& rel2) const
			{
				return equality<decltype(rel1.left)>()(rel1.left, rel2.left) && equality<decltype(rel1.right)>()(rel1.right, rel2.right);
			}
	};
}

namespace value_key_operators
{
	template<typename key_type>
	struct hash
	{
		public:
			size_t operator()(const key_type ptr) const
			{
				return key_hash(ptr);
			}
	};
	
	template<typename key_type>
	struct equality
	{
		public:
			bool operator()(const key_type& ptr1, const key_type& ptr2) const
			{
				return ptr1 == ptr2;
			}
	};
	
	template<typename Relation>
	struct relation_hash
	{
		public:
			size_t operator()(const Relation& rel) const
			{
				size_t seed = 0;
				boost::hash_combine(seed, rel.left);
				boost::hash_combine(seed, rel.right);
				return seed;
			}
	};
	
	template<typename Relation>
	struct relation_equality
	{
		public:
			bool operator()(const Relation& rel1, const Relation& rel2) const
			{
				return (rel1.left == rel2.left) && (rel1.right == rel2.right);
			}
	};
}

#endif /* KEY_OPERATORS_H */
