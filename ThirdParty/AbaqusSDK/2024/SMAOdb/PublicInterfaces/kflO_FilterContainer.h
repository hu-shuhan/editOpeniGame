//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kflO_FilterContainer_h
#define kflO_FilterContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kflO_FilterContainer.h
//
// Description:  Filter Container object on kflO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <kflO_Filter.h>
#include <cow_Btree.h>

COW_BTREE_CMP_DECL(odb_String, kflO_FilterCOW, kflO_FilterContainerSub);

// Forward declarations
class kflO_FilterContainerIT;

// Class definition

class kflO_FilterContainer  : public mem_AllocationOperators
{
    friend class kflO_FilterContainerIT;
    friend class kflO_ApiExtension;
public:

    kflO_FilterContainer();
    const kflO_Filter& operator[](const odb_String&) const;
    kflO_Filter& get(const odb_String&);
    const kflO_Filter& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kflO_FilterContainerSub container;
};

class kflO_FilterContainerIT  : public mem_AllocationOperators
{
public:
    kflO_FilterContainerIT(const kflO_FilterContainer& mc);

    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const kflO_Filter& currentValue() const;
    
private:
    kflO_FilterContainerSubIT iter;
};

#endif // kflO_FilterContainer_h
