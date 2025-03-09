//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef cow_Map_h
#define cow_Map_h

//
// Includes
//
#include <assert.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_COW.h>

//
// Forward declarations
//
template <class Key, class Value>
class cow_ConstMapIterator;

template <class Key, class Value>
class cow_ConstMapSortedIt;

//
// Class definition
//

/////////////////////////////////////////////
// DNOTE: This class represent a Many:One cow_Map.
/////////////////////////////////////////////

template <class Key, class Value>
struct cow_MapNode : public mem_AllocationOperators
{
public:
    Key           key;
    Value         val;
    typedef cow_COW<cow_MapNode<Key,Value>,
                    cow_Direct<cow_MapNode<Key,Value> > > NodePtr;
    NodePtr    l;
    NodePtr    r;

    inline cow_MapNode(const Key&, const Value&);
};


template <class Key, class Value>
class cow_Map : public mem_AllocationOperators
{

public:

    inline cow_Map();

    ////////////////////
    // Basic
    bool Remove(const Key&);
    bool Insert(const Key&, const Value&);

    ////////////////////
    // Lookup a value:
    const Value& ConstGet(const Key&) const;

    ////////////////////
    // Access a value
    Value& Get(const Key&);

    ////////////////////
    // Test membership
    bool IsMember(const Key&) const;

    // Return Size of Map
    int  Size() const { return size; }

    // Is Map empty?
    inline bool IsEmpty() const;

    // Clear the Map
    inline void Clear();

private:

    typedef cow_COW<cow_MapNode<Key,Value>,
                       cow_Direct<cow_MapNode<Key,Value> > > NodePtr;
    NodePtr head;
    int  size;

    friend class cow_ConstMapIterator<Key, Value>;
    friend class cow_ConstMapSortedIt<Key, Value>;

};

template <class Key>
inline bool cow_MapBit(const Key& key, int b);

//
// Const Iterator over Map
// NOT SAFE to change Map while iterating over it!
//
template <class Key, class Value>
class cow_ConstMapIterator : public mem_AllocationOperators
{
public:

    inline cow_ConstMapIterator(const cow_Map<Key, Value>& m);

    ~cow_ConstMapIterator() { 
        if (nodes) 
            mem_Free(nodes); 
    }

    void   First();

    void   Next() {
        if (!IsDone())
            index++;
    }

    bool   IsDone() const {
	assert(nodes && "Need to call First() on iterator.");
        return bool(nodes[index] == 0);
    }

    const Value&  CurrentValue() const {
	assert(nodes && "Need to call First() on iterator.");
        return nodes[index]->val;
    }

    const Key&    CurrentKey() const {
	assert(nodes && "Need to call First() on iterator.");
        return nodes[index]->key;
    }

private:
    // copy-constructor and assignment operator would be tricky, and
    // they don't seem very useful. So, wait until somebody needs them.
    //
    cow_ConstMapIterator(const cow_ConstMapIterator<Key, Value>&);
    cow_ConstMapIterator<Key, Value>& 
    operator=(const cow_ConstMapIterator<Key, Value>&);

    typedef cow_COW<cow_MapNode<Key,Value>,
                       cow_Direct<cow_MapNode<Key,Value> > > NodePtr;

    // buildup a list of nodes to iterate over; 
    // recursively used by First()
    void Nodes(const NodePtr&, int&);

    cow_Map<Key, Value>          map;
    int                          index;
    cow_MapNode<Key,Value> **    nodes;  
};

//
// Const SORTED Iterator over Map
// NOT SAFE to change Map while iterating over it!
//

template <class Key, class Value>
class cow_ConstMapSortedIt : public mem_AllocationOperators
{
public:

    // Sort order
    enum SortOrderEnm { ASCENDING, DESCENDING, CUSTOM };

    // Default ctor uses the default ascending SortCmp routine
    inline cow_ConstMapSortedIt(const cow_Map<Key, Value>& m);

    // Let's you choose a DESCENDING sort
    inline cow_ConstMapSortedIt(const cow_Map<Key, Value>& m, 
				SortOrderEnm s);

    // This ctor allows you to pass a custom compare routine which
    // will be used when sorting
    inline cow_ConstMapSortedIt(const cow_Map<Key, Value>& m,
				int (*cmpFun)(const void *, const void *));

    ~cow_ConstMapSortedIt() {
        if (nodes)
            mem_Free(nodes);
    }

    void   First();

    void   Next() {
        if (!IsDone())
            index++;
    }

    bool   IsDone() const {
	assert(nodes && "Need to call First() on iterator.");
        return bool(nodes[index] == 0);
    }

    const Value&  CurrentValue() const {
	assert(nodes && "Need to call First() on iterator.");
        return nodes[index]->val;
    }

    const Key&    CurrentKey() const {
	assert(nodes && "Need to call First() on iterator.");
        return nodes[index]->key;
    }

    // The Ascending comparison function. Specialize this if you
    // need to sort in some other fashion. Done as a member so
    // that the template does not have to be instantiated
    // separately
    static int AscSortCmp(const void *a, const void *b);

    // The Descending comparison function. Specialize this if you
    // need to sort in some other fashion. Done as a member so
    // that the template does not have to be instantiated
    // separately
    static int DescSortCmp(const void *a, const void *b);

private:
    // copy-constructor and assignment operator would be tricky, and
    // they don't seem very useful. So, wait until somebody needs them.
    //
    cow_ConstMapSortedIt(const cow_ConstMapSortedIt<Key, Value>&);
    cow_ConstMapSortedIt<Key, Value>& 
    operator=(const cow_ConstMapSortedIt<Key, Value>&);

    typedef cow_COW<cow_MapNode<Key,Value>,
                       cow_Direct<cow_MapNode<Key,Value> > > NodePtr;

    // buildup a list of nodes to iterate over; 
    // recursively used by First()
    void Nodes(const NodePtr&, int&);

    cow_Map<Key, Value>          map;
    int                          index;
    cow_MapNode<Key,Value> **    nodes;  

    // Default is an ascending sort
    SortOrderEnm                 sortOrder;

    // The comparison function to use
    int (*compar)(const void *, const void *);

};


//
// Inlines
//

template <class Key, class Value>
inline cow_MapNode<Key,Value>::cow_MapNode(
    const Key& k, const Value& v
)
    : key(k), val(v), l(0), r(0)
{ }

template <class Key, class Value>
inline cow_Map<Key,Value>::cow_Map()
    : head(0), size(0)
{ }

// Is Map empty?
template <class Key, class Value>
inline bool cow_Map<Key,Value>::IsEmpty() const 
{ return head.IsNull(); }

template <class Key, class Value>
inline void cow_Map<Key,Value>::Clear()
{
    // this should be all that is needed
    head = 0;
    size = 0;
}

template <class Key, class Value>
inline cow_ConstMapIterator<Key,Value>::cow_ConstMapIterator(
    const cow_Map<Key, Value>& m
)
    : map(m), index(0), 
      nodes(0)
{ }

template <class Key, class Value>
inline cow_ConstMapSortedIt<Key,Value>::cow_ConstMapSortedIt(
    const cow_Map<Key, Value>& m
)
    : map(m), index(0), sortOrder(ASCENDING),
      nodes(0), compar(cow_ConstMapSortedIt<Key,Value>::AscSortCmp)
{ }

template <class Key, class Value>
inline cow_ConstMapSortedIt<Key,Value>::cow_ConstMapSortedIt(
    const cow_Map<Key, Value>& m,
    SortOrderEnm s
)
    : map(m), index(0), sortOrder(s),
      nodes(0), 
      compar(s == DESCENDING ? 
	     cow_ConstMapSortedIt<Key,Value>::DescSortCmp :
	     cow_ConstMapSortedIt<Key,Value>::AscSortCmp)
{ }

template <class Key, class Value>
inline cow_ConstMapSortedIt<Key,Value>::cow_ConstMapSortedIt(
    const cow_Map<Key, Value>& m,
    int (*cmpFun)(const void *, const void *)
)
    : map(m), index(0), sortOrder(CUSTOM),
      nodes(0), compar(cmpFun)
{ }

//
// The COW_MAP_DECL macro declares a cow_Map and 
// forward declares a MapIter. You choose the NAME.
//
// The COW_MAP_ITER_DECL macro declares the cow_Map *AND* 
// the Iterator. Use this one if you need an iterator. The iterator
// name is the name you pass, concatenated with 'IT'.
//
// CHOOSE one *OR* the other!
// Use the *_DECL macros in your .h file
//
#define COW_MAP_FWDL(KEY,VALUE,NAME) \
typedef cow_MapNode<KEY,VALUE> NAME ## ND; \
typedef cow_ConstMapIterator<KEY,VALUE> NAME ## IT; \
typedef cow_ConstMapSortedIt<KEY,VALUE> NAME ## SIT; \
COW_COW_FWDL(NAME ## ND) \
typedef cow_Map<KEY,VALUE> NAME;

#ifdef SMA_NO_TGEN
#define COW_MAP_EXTL(KEY,VALUE,NAME) \
COW_COW_EXTL(NAME ## ND) \
extern template class cow_MapNode<KEY,VALUE>; \
extern template class cow_Map<KEY,VALUE>;
#else
#define COW_MAP_EXTL(KEY,VALUE,NAME) \
COW_COW_EXTL(NAME ## ND)
#endif

#define COW_MAP_DECL(KEY,VALUE,NAME) \
COW_MAP_FWDL(KEY,VALUE,NAME) \
COW_MAP_EXTL(KEY,VALUE,NAME)

#define COW_MAP_ITER_FWDL(KEY,VALUE,NAME) \
COW_MAP_FWDL(KEY,VALUE,NAME) \
typedef cow_ConstMapIterator<KEY,VALUE> NAME ## IT;

#ifdef SMA_NO_TGEN
#define COW_MAP_ITER_EXTL(KEY,VALUE,NAME) \
COW_MAP_EXTL(KEY,VALUE,NAME) \
extern template class cow_ConstMapIterator<KEY,VALUE>;
#else
#define COW_MAP_ITER_EXTL(KEY,VALUE,NAME) \
COW_MAP_EXTL(KEY,VALUE,NAME)
#endif

#define COW_MAP_ITER_DECL(KEY,VALUE,NAME) \
COW_MAP_ITER_FWDL(KEY,VALUE,NAME) \
COW_MAP_ITER_EXTL(KEY,VALUE,NAME)

#define COW_MAP_SORTED_ITER_FWDL(KEY,VALUE,NAME) \
COW_MAP_FWDL(KEY,VALUE,NAME) \
typedef cow_ConstMapSortedIt<KEY,VALUE> NAME ## SIT;

#ifdef SMA_NO_TGEN
#define COW_MAP_SORTED_ITER_EXTL(KEY,VALUE,NAME) \
COW_MAP_EXTL(KEY,VALUE,NAME) \
extern template class cow_ConstMapSortedIt<KEY,VALUE>;
#else
#define COW_MAP_SORTED_ITER_EXTL(KEY,VALUE,NAME) \
COW_MAP_EXTL(KEY,VALUE,NAME)
#endif

#define COW_MAP_SORTED_ITER_DECL(KEY,VALUE,NAME) \
COW_MAP_SORTED_ITER_FWDL(KEY,VALUE,NAME) \
COW_MAP_SORTED_ITER_EXTL(KEY,VALUE,NAME)

//
// These macros go in the .C file.
//
// CHOOSE one *OR* the other!
// Use the one that matches the _DECL that you chose.
//
// Remember to include cow_Map.T!
//
#define COW_MAP_IMPL(KEY,VALUE,NAME) \
COW_COW_IMPL(NAME ## ND) \
template class cow_MapNode<KEY,VALUE>; \
template class cow_Map<KEY,VALUE>;

#define COW_MAP_ITER_IMPL(KEY,VALUE,NAME) \
COW_MAP_IMPL(KEY,VALUE,NAME) \
template class cow_ConstMapIterator<KEY,VALUE>;

#define COW_MAP_SORTED_ITER_IMPL(KEY,VALUE,NAME) \
COW_MAP_IMPL(KEY,VALUE,NAME) \
template class cow_ConstMapSortedIt<KEY,VALUE>;



#endif  // #ifdef cow_Map_h
