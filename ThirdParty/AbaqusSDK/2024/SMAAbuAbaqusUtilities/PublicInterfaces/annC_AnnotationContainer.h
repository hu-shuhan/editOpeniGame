//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_AnnotationContainer_h
#define annC_AnnotationContainer_h

// Begin local includes
#include <mdl_Repository.h>
#include <annC_MapString2AnnotationCow.h>
#include <cow_ListString.h>

#include <cls_Uid.h>

//
// Forward declarations
//
class annC_AnnotationContainerIT;
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;


//
// Class definition
//
class annC_AnnotationContainer
    : public mdl_Repository
{
    friend class annC_AnnotationContainerIT;
public:
    annC_AnnotationContainer();
    virtual ~annC_AnnotationContainer();

    virtual mdl_Extension *Copy() const;

    // Interface
    bool Remove( const cow_String &name);
    void Insert( const cow_String &name, const annC_Annotation &ann );
    void Insert( const cow_String &name, annC_Annotation *ann ); // adopt insert method
    bool ChangeKey( const cow_String &fromName, const cow_String &toName );
    void Clear();

    bool BringToFront( const cow_String &name );
    bool SendToBack( const cow_String &name );
    bool BringForward( const cow_String &name );
    bool SendBackward( const cow_String &name );

    const cow_String &GetFirst() const { return m_map.GetFirst(); }
    const cow_String &GetLast() const { return m_map.GetLast(); }
    const cow_String &GetNext( const cow_String &key ) const { return m_map.GetNext( key ); }
    const cow_String &GetPrevious( const cow_String &key ) const { return m_map.GetPrevious( key ); }

    bool MoveBefore( const cow_String &key, const cow_String &other );
    bool MoveAfter( const cow_String &key, const cow_String &other );

    unsigned GetId( const cow_String &name ) const { return m_map.GetId(name); }
    cow_String GetName( unsigned id ) const { return m_map.GetKey(id); }

    bool IsMember( const cow_String &name ) const { return m_map.IsMember( name ); }
    bool IsMember( unsigned id ) const { return m_map.IsMember( id ); }

    bool IsEmpty() const { return m_map.Size() == 0; }
    int Size() const { return m_map.Size(); }

    annC_Annotation &Get( const cow_String &name );
    const annC_Annotation &ConstGet( const cow_String &name ) const;

    annC_Annotation &Get( unsigned id );
    const annC_Annotation &ConstGet( unsigned id ) const;

    cow_ListString Keys() const;

    const annC_MapString2AnnotationCow &ConstGetMap() const { return m_map; }

public:  // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

public:  // DDB interface

    // As a collection
    annC_AnnotationContainer(const cls_ReadVisitor& rv, bool asCollection);
    void DBWrite(const cls_WriteVisitor& wv, bool asCollection) const;
    // As an object containing a collection
    annC_AnnotationContainer(const cls_ReadVisitor& rv);
    void DBWrite(const cls_WriteVisitor& wv) const;

    static void InitializeMetadata(cls_ClassRegistrar &reg);
    void NewUid();

private:

    void PostDeltaRename( const cow_String &fromName, const cow_String &toName ) const;
    void PostDeltaInsert( const cow_String &name ) const;
    void PostDeltaRemove( const cow_String &name );
    void PostDeltaMove( const cow_String &name, int index );

    int IndexOf( const cow_String &name ) const;

    cls_Uid m_ClsUid;
    annC_MapString2AnnotationCow m_map;
};

class annC_AnnotationContainerIT
    : public annC_MapString2AnnotationCowIter
{
public:
    annC_AnnotationContainerIT( const annC_AnnotationContainer &c )
        : annC_MapString2AnnotationCowIter(c.m_map) {}
        
    const annC_Annotation &CurrentValue() const
        { return annC_MapString2AnnotationCowIter::CurrentValue().ConstGet(); }
};



#endif
