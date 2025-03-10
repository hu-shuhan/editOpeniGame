//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef cow_Btree_h
#define cow_Btree_h

//
// Includes
//
#include <assert.h>

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omi_types.h>
#include <cow_COW.h>
// End local includes

//
// Forward declarations
//
template <class Key, class Value>
class cow_BtreeIterator;

template <class Key, class Value>
class cow_Btree;


//
// Class definition
//

/////////////////////////////////////////////
// DNOTE: This class represent a Many:One cow_Btree.
/////////////////////////////////////////////
template <class Key, class Value>
class cow_BtreeNode : public mem_AllocationOperators
{
public:

    typedef cow_COW<cow_BtreeNode<Key,Value>,
                cow_Direct<cow_BtreeNode<Key,Value> > > cow_BtreeNodeCow;

    cow_BtreeNode(const Key &key, const Value &value );
    
    cow_BtreeNode(const cow_BtreeNode<Key,Value> &node ) :
        nodeKey(node.nodeKey),
        nodeValue(node.nodeValue),
        leftNode(0),
        rightNode(0),
        maxDepth(node.maxDepth)
    {
        if( node.leftNode ) leftNode = new cow_BtreeNodeCow( *(node.leftNode) );
        if( node.rightNode ) rightNode = new cow_BtreeNodeCow( *(node.rightNode) );
    }
   

    ~cow_BtreeNode()
    {
        if( leftNode ) delete leftNode;
        if( rightNode ) delete rightNode;
    }

    bool InsertUnlessMember( const Key &key, const Value &value );
    bool Remove( const Key &key, cow_BtreeNode<Key,Value> &parent );

    const cow_BtreeNode<Key,Value>* ConstFind( const Key& ) const;
    cow_BtreeNode<Key,Value>* Find( const Key& );

    int MaxDepth() const { return maxDepth; }

    cow_BtreeNodeCow* Balance( cow_BtreeNodeCow* cowofthis );

    const Value& ConstGetValue() const { return nodeValue; }
    Value&       GetValue() { return nodeValue; }
    const Key& ConstGetKey() const { return nodeKey; }

private:
    Value  nodeValue;
    Key    nodeKey;
    cow_BtreeNodeCow* leftNode;
    cow_BtreeNodeCow* rightNode;
    int                      maxDepth;

    int Compare(const Key &key1, const Key &key2) const;

    void RemoveNode( cow_BtreeNode<Key,Value>* ptr,
		     cow_BtreeNodeCow* replacement);

    void SetMaxDepth()
    {
	int depth = leftNode != 0 ? leftNode->ConstGet().MaxDepth() + 1 : 0;
	maxDepth = rightNode != 0 ? rightNode->ConstGet().MaxDepth() + 1 : 0;
	if( depth > maxDepth ) maxDepth = depth;
    }
    
    friend class cow_Btree<Key,Value>;
    friend class cow_BtreeIterator<Key,Value>;

};


template <class Key, class Value>
class cow_Btree : public mem_AllocationOperators
{

public:

    cow_Btree();
    cow_Btree( const cow_Btree<Key,Value>& );

    ~cow_Btree() 
    {
        if( treeNode ) delete treeNode;
    }

    const cow_Btree<Key,Value>& operator=( const cow_Btree<Key,Value>& );
    cow_Btree<Key,Value>* Copy() const { return new cow_Btree<Key,Value>(*this); }

    ////////////////////
    // Basic
    bool Remove(const Key&);
    bool InsertUnlessMember(const Key&, const Value&);

    ////////////////////
    // Lookup a value:
    const Value& ConstGet(const Key &key) const
    {
	 const cow_BtreeNode<Key,Value>* node = ConstFind(key);
        if( node )
	    return node->ConstGetValue();
	return nullValue;
    }

    ////////////////////
    // Access a value
    Value& Get(const Key &key)
    {
	 cow_BtreeNode<Key,Value>* node = Find(key);
	 
        if( node )
	    return node->GetValue();
	return nullValue;
    }

    ////////////////////
    // Test membership
    bool IsMember(const Key &key) const
    {
	 if(ConstFind(key) != 0)
	      return true;
	 else
	      return false;
    }

    // Return Size of Map
    int  Size() const { return nodeCount; }; 

    // Is Map empty?
    bool IsEmpty() const 
    { 
	 if(nodeCount == 0)
	      return true; 
	 else
	      return false;
    }
     

    // Clear the Map
    void Clear()
    {
        delete treeNode;
	treeNode = 0;
	nodeCount = 0;
    }

    // This comparison method requires operator < and > on Key.  You may
    // alternatively specialize this method with another implementation.
    static int Compare(const Key &key1, const Key &key2) {
        return (key1 < key2 ? 1 : (key1 > key2 ? -1 : 0));
    }

    void SetNullValue( const Value &value ) { nullValue = value; }
    const Value& NullValue() const { return nullValue; }

    int Depth() const { return treeNode ? treeNode->ConstGet().MaxDepth() : 0; }

private:
    typedef cow_COW<cow_BtreeNode<Key,Value>,
                cow_Direct<cow_BtreeNode<Key,Value> > > cow_BtreeNodeCow;

    typedef cow_BtreeNode<Key,Value> cow_BtreeNodeInst;

    friend class cow_BtreeIterator<Key,Value>;

    const cow_BtreeNode<Key,Value>* ConstFind( const Key &key) const;
    cow_BtreeNode<Key,Value>* Find( const Key &key );

    // Get the *default* null value; used during construction.
    // You must specialize this if Value has no default constructor.
    Value GetNullValue();

    cow_BtreeNodeCow* treeNode;
    int nodeCount;
    Value nullValue;

};


template<class Key, class Value>
inline int
cow_BtreeNode<Key,Value>::Compare(const Key &key1, const Key &key2) const
{
    return cow_Btree<Key,Value>::Compare(key1, key2);
}


template <class Key, class Value>
class cow_BtreeNodeLink : public mem_AllocationOperators
{
public:
    typedef cow_COW<cow_BtreeNode<Key,Value>,
                cow_Direct<cow_BtreeNode<Key,Value> > > cow_BtreeNodeCow;

    cow_BtreeNodeLink(  const cow_BtreeNodeCow* node,
                         cow_BtreeNodeLink<Key,Value> *link ) :
        theNodeCow(node),
	nextLink(link)
    {
    }
    ~cow_BtreeNodeLink() {}

    const cow_BtreeNodeCow* Node() { return theNodeCow; }

    cow_BtreeNodeLink<Key,Value> *Next() { return nextLink; }
private:
    const cow_BtreeNodeCow* theNodeCow;
    cow_BtreeNodeLink<Key,Value> *nextLink;
};



template <class Key, class Value>
class cow_BtreeIterator : public mem_AllocationOperators
{

public:
    cow_BtreeIterator( const cow_Btree<Key,Value> &tree );
    ~cow_BtreeIterator();

    void First();
    void Next();
    void Last();
    void Previous();

    const Value& CurrentValue() const {
        return nodeStack->Node()->ConstGet().ConstGetValue(); 
    }
    const Key& CurrentKey() const {
        return nodeStack->Node()->ConstGet().ConstGetKey();
    }

    bool IsDone() const { return (nodeStack ? false : true); }
     
private:
    typedef cow_COW<cow_BtreeNode<Key,Value>,
                cow_Direct<cow_BtreeNode<Key,Value> > > cow_BtreeNodeCow;

    const cow_Btree<Key,Value> &treeRef;
    cow_BtreeNodeLink<Key,Value> *nodeStack;

    void Push( const cow_BtreeNodeCow* node )
    {
        nodeStack = new cow_BtreeNodeLink<Key,Value>(node,nodeStack);
    }
    void Pop()
    {
       cow_BtreeNodeLink<Key,Value> *tmp = nodeStack;
	 
       if( tmp )
       {
          nodeStack = tmp->Next();
	  delete tmp;
       }
    }
    void ClearStack()
    {
        while( nodeStack ) Pop();
    }
};


////////////////////////////////////////////////////////////////////////
//
// Macros
//

#define COW_BTREE_FWDL( KEY, VALUE, NAME ) \
typedef cow_BtreeNode<KEY,VALUE> NAME ## ND; \
COW_COW_FWDL( NAME ## ND ) \
typedef cow_Btree<KEY,VALUE> NAME; \
typedef cow_BtreeIterator<KEY,VALUE> NAME ## IT; \
typedef cow_BtreeIterator<KEY,VALUE> NAME ## SIT;

#ifdef SMA_NO_TGEN
#define COW_BTREE_EXTL( KEY, VALUE, NAME ) \
COW_COW_EXTL( NAME ## ND ) \
extern template class cow_BtreeNode<KEY,VALUE>; \
extern template class cow_Btree<KEY,VALUE>; \
extern template class cow_BtreeNodeLink<KEY,VALUE>; \
extern template class cow_BtreeIterator<KEY,VALUE>;
#else
#define COW_BTREE_EXTL( KEY, VALUE, NAME ) \
COW_COW_EXTL( NAME ## ND )
#endif

#define COW_BTREE_CMP_EXTL( KEY, VALUE, NAME ) \
template <> inline int cow_Btree<KEY,VALUE>::Compare(const KEY &k1, const KEY &k2); \
COW_BTREE_EXTL( KEY, VALUE, NAME ) \
static const bool NAME ## _ist = true;

#define COW_BTREE_DECL( KEY, VALUE, NAME ) \
COW_BTREE_FWDL( KEY, VALUE, NAME ) \
COW_BTREE_EXTL( KEY, VALUE, NAME )

#define COW_BTREE_CMP_DECL( KEY, VALUE, NAME ) \
COW_BTREE_FWDL( KEY, VALUE, NAME ) \
COW_BTREE_CMP_EXTL( KEY, VALUE, NAME )


#define COW_BTREE_IMPL( KEY, VALUE, NAME ) \
COW_COW_IMPL( NAME ## ND ) \
template class cow_BtreeNode<KEY,VALUE>; \
template class cow_Btree<KEY,VALUE>; \
template class cow_BtreeNodeLink<KEY,VALUE>; \
template class cow_BtreeIterator<KEY,VALUE>;


// Same as above, but replace default comparison method with 'BtreeCompare',
// which must be defined somewhere (non-template, preferably inline).
// (A number of these for common types are defined in cow_BtreeCompare.h.)
#define COW_BTREE_CMP_IMPL( KEY, VALUE, NAME ) \
template <> inline int cow_Btree<KEY,VALUE>::Compare(const KEY &k1, const KEY &k2) \
{ \
  return BtreeCompare(k1,k2); \
} \
COW_BTREE_IMPL( KEY, VALUE, NAME ) \
static_assert(NAME ## _ist);


#endif
