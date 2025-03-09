//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_MapString2AnnotationCow_h
#define annC_MapString2AnnotationCow_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <mem_AllocationOperatorsTiny.h>
#include <annC_ListPairStringAnnotation.h>
#include <cow_Map.h>
#include <cow_ArrayCOW.h>

// Forward declarations

class annC_MapString2AnnotationCowImplIter;
class annC_MapString2AnnotationCowIter;

class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

typedef annC_ListPairStringAnnotation::Node *annC_ListPairStringAnnotationNodeP;
COW_MAP_DECL(cow_String, annC_ListPairStringAnnotationNodeP, annC_MapString2ListNodeP);
COW_MAP_DECL(int, annC_ListPairStringAnnotationNodeP, annC_MapInt2ListNodeP);


/////////////////////////////////////////////////////////////////////////////////////
//
// This class is an implementation of a map that allows clients to
// maintain keys in a specific order. Despite possessing of that extra capability,
// access to values has the same complexity as in an ordinary map.
//
// Objects in the repository can be retrieved by a Key (name) 
// or by an ID. Both ways of accessing are equally efficient.
// Object IDs are unique within all repositories in session.
// It is not possible to change an object's ID, therefore
// IDs serve as a good reference, immune to renames.
//
// Because copying of objects of this class is expensive, we do not expose
// this class directly, but rather through its COW-ed version: annC_MapString2AnnotationCow
// defined below.
//
class annC_MapString2AnnotationCowImpl  : public mem_AllocationOperators
{
    friend class annC_MapString2AnnotationCowImplIter;
public:
    typedef cow_String Key;
    typedef annC_AnnotationCOW Value;

    annC_MapString2AnnotationCowImpl();
    annC_MapString2AnnotationCowImpl( const annC_MapString2AnnotationCowImpl & );
    void operator = ( const annC_MapString2AnnotationCowImpl & );

    void Remove( const Key &key );
    void PushFront( const Key &key, const Value &value );
    void PushBack( const Key &key, const Value &value );
    void InsertBefore( const Key &key, const Value &value, const Key &other );
    void InsertAfter( const Key &key, const Value &value, const Key &other );
    void ChangeKey( const Key &from, const Key &to );

    const Key &GetFirst() const;
    const Key &GetLast() const;
    const Key &GetNext( const Key &key ) const;
    const Key &GetPrevious( const Key &key ) const;
    
    void MoveBefore( const Key &key, const Key &other );
    void MoveAfter( const Key &key, const Key &other );

    const Value &ConstGet( const Key &key ) const;
    Value &Get( const Key &key );

    const Value &ConstGet( unsigned id ) const;
    Value &Get( unsigned id );

    bool IsMember( const Key &key ) const;
    bool IsMember( unsigned id ) const;

    unsigned GetId( const Key &key ) const;
    const Key &GetKey( unsigned id ) const;

    int Size() const { return m_map.Size(); }
    bool IsEmpty() const { return m_map.IsEmpty(); }
    void Clear();

private:
    typedef annC_PairStringAnnotation Pair;

    annC_ListPairStringAnnotation m_list;
    annC_MapString2ListNodeP      m_map;
    annC_MapInt2ListNodeP         m_idMap;
};

COW_ARRAYCOW2_DECL(annC_MapString2AnnotationCowImpl, cow_Direct);

class annC_MapString2AnnotationCowImplIter  : public mem_AllocationOperators
{
public:
    typedef annC_MapString2AnnotationCowImpl::Key Key;
    typedef annC_MapString2AnnotationCowImpl::Value Value;

    annC_MapString2AnnotationCowImplIter( const annC_MapString2AnnotationCowImpl &m )
        : m_list(m.m_list), m_currentNode(NULL) {}

    void First() { m_currentNode = m_list.GetFirst(); }
    void Last() { m_currentNode = m_list.GetLast(); }
    void Next() { m_currentNode = m_list.GetNext( m_currentNode ); }
    void Previous() { m_currentNode = m_list.GetPrevious( m_currentNode ); }
    bool IsDone() { return !m_currentNode; }

    const Key &CurrentKey() const { return m_list.ConstGet( m_currentNode ).key; }
    unsigned CurrentId() const { return m_list.NodeId( m_currentNode ); }
    const Value &CurrentValue() const { return m_list.ConstGet( m_currentNode ).value; }

private:
    const annC_ListPairStringAnnotation &m_list;
    annC_ListPairStringAnnotation::Node *m_currentNode;
};


////////////////////////////////////////////////////////////////////////////////
//
// This class is an equivalent of cow_Map<Key,Value>, except that
// the order of keys can be freely rearranged by the client.
//
class annC_MapString2AnnotationCow  : public mem_AllocationOperators
{
  friend class annC_MapString2AnnotationCowIter;
public:
  typedef cow_String Key;
  typedef annC_AnnotationCOW Value;

  annC_MapString2AnnotationCow(); 
  annC_MapString2AnnotationCow(const cls_ReadVisitor& rv );

  void Remove( const Key &key ); 
  void PushFront( const Key &key, const Value &value ); 
  void PushBack( const Key &key, const Value &value ); 
  void InsertBefore( const Key &key, const Value &value, const Key &other );
  void InsertAfter( const Key &key, const Value &value, const Key &other );
  void ChangeKey( const Key &from, const Key &to );

  const Key & GetFirst() const;
  const Key & GetLast() const;

  // Note: These will wrap around
  const Key & GetNext( const Key &key ) const; 
  const Key & GetPrevious( const Key &key ) const; 

  void MoveBefore( const Key &key, const Key &other ); 
  void MoveAfter( const Key &key, const Key &other );

  const Value & ConstGet( const Key &key ) const; 
  Value &       Get( const Key &key );

  const Value & ConstGet( unsigned id ) const;
  Value &       Get( unsigned id );

  bool IsMember( const Key &key ) const;
  bool IsMember( unsigned id ) const;

  unsigned int GetId( const Key &key ) const; 
  const Key &  GetKey( unsigned id ) const; 

  int      Size() const; 
  bool     IsEmpty() const;
  void     Clear();

  void     DBWrite(const cls_WriteVisitor& wv) const;

private:
    annC_MapString2AnnotationCowImplCOW  m_map;
    cls_Uid                              m_ClsUid;
};

// Iterator class for the map.
// Note: Iterator access is faster than using map member functions for iteration!
class annC_MapString2AnnotationCowIter
    : public annC_MapString2AnnotationCowImplIter
{
public:
    annC_MapString2AnnotationCowIter( const annC_MapString2AnnotationCow &m )
        : annC_MapString2AnnotationCowImplIter( m.m_map.ConstGet() ) {}
};


#endif
