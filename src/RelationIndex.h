
#ifndef RELATION_INDEX_H
#define RELATION_INDEX_H

template<typename ModelClassPtr, typename KeyModelClassPtr>
class RelationIndex;

#include "HashIndex.h"
#include "KeyOperators.h"

template<typename ModelClassPtr, typename KeyModelClassPtr>
class RelationIndex : public HashIndex< ModelClassPtr, KeyModelClassPtr, value_key_operators::hash<KeyModelClassPtr>, value_key_operators::equality<KeyModelClassPtr> >
{
	virtual bool isRelationIndex() const { return true; }
};

#endif /* RELATION_INDEX_H */
