//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_ListPairStringAnnotation_h
#define annC_ListPairStringAnnotation_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <annC_Annotation.h>
#include <cow_String.h>

// Forward declarations
class annC_ListPairStringAnnotationNode;

class annC_PairStringAnnotation  : public mem_AllocationOperators
{
public:
    typedef cow_String Key;
    typedef annC_AnnotationCOW Value;

    annC_PairStringAnnotation( const Key &k, const Value &v )
        : key(k), value(v) {}

    Key key;
    Value value;
};

// This class is a list of (name,annotationCOW) pairs. It is used internally
// by the annotation container to maintain stacking order.
// This class is not COW-ed in any way, so copying may be expensive.
class annC_ListPairStringAnnotation  : public mem_AllocationOperators
{
public:
    typedef annC_ListPairStringAnnotationNode Node;
    typedef annC_PairStringAnnotation Value;

    annC_ListPairStringAnnotation();
    annC_ListPairStringAnnotation( const annC_ListPairStringAnnotation & );
    void operator = ( const annC_ListPairStringAnnotation & );
    ~annC_ListPairStringAnnotation();

    const Value &ConstGet( const Node *node ) const;
    Value &Get( Node *node );
    unsigned NodeId( const Node *node ) const;

    Node *GetFirst() const;
    Node *GetLast() const;
    Node *GetNext( Node *node ) const;
    Node *GetPrevious( Node *node ) const;

    // Inserts a value before "node". Returns a new node.
    Node *InsertBefore( Node *node, const Value &value );
    Node *InsertAfter( Node *node, const Value &value );

    // Removes and deletes the node.
    void Remove( Node *node );

    void MoveBefore( Node *node, Node *other );
    void MoveAfter( Node *node, Node *other );

    // Clears contents.
    void Clear();

private:
    void LinkBefore( Node *node, Node *other );
    void LinkAfter( Node *node, Node *other );
    void LinkBetween( Node *node, Node *left, Node *right );
    void Unlink( Node *node );

    Node *m_first;
    Node *m_last;
    unsigned m_lastId;
};

#endif
