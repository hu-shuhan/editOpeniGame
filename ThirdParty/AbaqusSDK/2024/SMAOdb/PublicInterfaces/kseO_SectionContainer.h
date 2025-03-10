//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kseO_SectionContainer_h
#define kseO_SectionContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kseO_SectionContainer.h
//
// Description:  Section Container object on kseO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <odb_Section.h>
#include <cow_Btree.h>

COW_BTREE_CMP_DECL(odb_String, odb_SectionCOW, kseO_SectionContainerSub);

// Forward declarations
class kseO_SectionContainerIT;

// Class definition

class kseO_SectionContainer  : public mem_AllocationOperators
{
    friend class kseO_SectionContainerIT;
    friend class kseO_ApiExtension;
public:

    kseO_SectionContainer();
    const odb_Section& operator[](const odb_String&) const;
    odb_Section& get(const odb_String&);
    const odb_Section& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kseO_SectionContainerSub container;
};

class kseO_SectionContainerIT  : public mem_AllocationOperators
{
public:
    kseO_SectionContainerIT(const kseO_SectionContainer& mc);

    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const odb_Section& currentValue() const;
    
private:
    kseO_SectionContainerSubIT iter;
};

#endif // kseO_SectionConatiner_h
