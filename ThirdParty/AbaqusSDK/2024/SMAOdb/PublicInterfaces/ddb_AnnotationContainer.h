//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_AnnotationContainer_h
#define ddb_AnnotationContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: ddb_AnnotationContainer.h
//
// Description:  Root uddults object on ddb
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <annC_Annotation.h>
#include <udd_AnnotationShortcut.h>
#include <annC_AnnotationContainer.h>
#include <ddb_Text.h>
#include <ddb_Arrow.h>
#include <cow_ListString.h>
#include <cow_2DArrayString.h>
#include <ddb_Annotation.h>

// Forward declarations
class ddb_AnnotationContainerIT;

// Class definition

class ddb_AnnotationContainer  : public mem_AllocationOperators
{
public:
    ddb_AnnotationContainer(const udd_AnnotationContainerOdbShortcut&);

    ddb_Annotation operator[](const cow_String&) const;
    ddb_Annotation operator[](unsigned id) const;
    void insert(const cow_String&, const ddb_Annotation&);
    void remove(const cow_String&);

    ddb_Annotation CreateAnnotationInContainer(const cow_String& name, const annC_Annotation& annotation );
    ddb_Arrow CreateArrowInContainer(const cow_String& name);
    ddb_Text CreateTextInContainer(const cow_String& name);

    void bringToFront( const cow_String &name );
    void sendToBack( const cow_String &name );
    void bringForward( const cow_String &name );
    void sendBackward( const cow_String &name );
    void moveBefore( const cow_String &name, const cow_String &other );
    void moveAfter( const cow_String &name, const cow_String &other );

    int size() const;
    bool isEmpty() const;
    bool isMember(const cow_String& name) const;
    bool isMember(unsigned id) const;

    unsigned getId(const cow_String &name) const;
    cow_String getName(unsigned id) const;

    cow_ListString keys() const;
    cow_2DArrayString summary() const;

    ddb_AnnotationContainerIT createIterator() const;

    udd_AnnotationContainerOdbShortcut Shortcut() const { return shortcut; }

private:
    udd_AnnotationContainerOdbShortcut shortcut;
};

class ddb_AnnotationContainerIT  : public mem_AllocationOperators
{
public:
     ddb_AnnotationContainerIT(const annC_AnnotationContainer& cont,
			       const udd_AnnotationContainerOdbShortcut& sc);

     inline void first();
     inline void next();
     inline bool isDone();

     inline cow_String currentKey() const;
     inline ddb_Annotation currentValue() const;

private:
     annC_AnnotationContainerIT iter;
     udd_AnnotationContainerOdbShortcut shortcut;
};

inline
void
ddb_AnnotationContainerIT::first()
{
     iter.First();
}

inline
void
ddb_AnnotationContainerIT::next()
{
     iter.Next();
}

inline
bool
ddb_AnnotationContainerIT::isDone()
{
     return iter.IsDone();
}

inline
cow_String
ddb_AnnotationContainerIT::currentKey() const
{
     return iter.CurrentKey();
}

inline
ddb_Annotation
ddb_AnnotationContainerIT::currentValue() const
{
    return ddb_Annotation(
        udd_AnnotationInAnnotationContainerOdbShortcut(shortcut, iter.CurrentId()),
        iter.CurrentId());
}


#endif 
