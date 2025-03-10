//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kbpO_SectionContainer_h
#define kbpO_SectionContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kbpO_SectionContainer.h
//
// Description:  Section Container object on kbpO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <cow_Btree.h>
#include <kbpO_Profile.h>

COW_BTREE_CMP_DECL(odb_String, kbpO_ProfileCOW, kbpO_ProfileContainerSub);

// Forward declarations
class kbpO_ProfileContainerIT;

// Class definition

class kbpO_ProfileContainer  : public mem_AllocationOperators
{
    friend class kbpO_ProfileContainerIT;
    friend class kseO_ApiExtension;
public:

    kbpO_ProfileContainer();
    const kbpO_Profile& operator[](const odb_String&) const;
    kbpO_Profile& get(const odb_String&);
    const kbpO_Profile& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kbpO_ProfileContainerSub container;
};

class kbpO_ProfileContainerIT  : public mem_AllocationOperators
{
public:
    kbpO_ProfileContainerIT(const kbpO_ProfileContainer& mc);

    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const kbpO_Profile& currentValue() const;
    
private:
    kbpO_ProfileContainerSubIT iter;
};

#endif // kbpO_SectionConatiner_h
