//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_AnnotationRepository_h
#define odb_AnnotationRepository_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Annotation.h>
#include <ddb_AnnotationContainer.h>
#include <cow_Btree.h>
// End local includes

COW_BTREE_DECL(int, odb_Annotation, odb_AnnotationCache);


class odb_AnnotationRepository  : public mem_AllocationOperators
{
public:

    void insert(const odb_String& name, const odb_Annotation &ann);
    void yank(const odb_String& name);

    odb_Annotation& operator[](const odb_String& name);
    odb_Annotation operator[](const odb_String& name) const;
    odb_Annotation& get(const odb_String& name);
    odb_Annotation constGet(const odb_String& name) const;

    int  size() const { return m_container.size(); }
    bool isEmpty() const { return m_container.isEmpty(); }
    bool isMember(const odb_String& name) const { return m_container.isMember(name); }

    void bringToFront( const odb_String &name );
    void sendToBack( const odb_String &name );
    void bringForward( const odb_String &name );
    void sendBackward( const odb_String &name );
    void moveBefore( const odb_String &name, const odb_String &other );
    void moveAfter( const odb_String &name, const odb_String &other );

public: // Unsupported an undocumented
    odb_AnnotationRepository( ddb_AnnotationContainer container );

    const ddb_AnnotationContainer &constRepository() const { return m_container; }
    odb_Annotation &getAnnotation( unsigned id );

private:
    friend class odb_AnnotationRepositoryIT;
    void CheckKey( const odb_String& name) const;

    ddb_AnnotationContainer m_container;
    odb_AnnotationCache m_cache;
};

class odb_AnnotationRepositoryIT  : public mem_AllocationOperators
{
public:
    odb_AnnotationRepositoryIT( const odb_AnnotationRepository &repo )
        : m_iter( repo.m_container.createIterator() )
    {}

    void first() { m_iter.first(); }
    void next() { m_iter.next(); }
    bool isDone() { return m_iter.isDone(); }

    odb_String currentKey() const { return m_iter.currentKey(); }
    odb_Annotation currentValue() const { return odb_Annotation( m_iter.currentValue() ) ;}

private:
    ddb_AnnotationContainerIT m_iter;
};

#endif // #ifndef odb_AnnotationRepository_h

