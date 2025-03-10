//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2015
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kbeO_ConnectorBehaviorContainer_h
#define kbeO_ConnectorBehaviorContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kbeO_ConnectorBehaviorContainer.h
//
// Description:  ConnectorBehaviorOption Container object on kamO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <kbeO_ConnectorBehaviorOption.h>
#include <cow_Btree.h>

COW_BTREE_CMP_DECL(odb_String, kbeO_ConnectorBehaviorOptionCOW, kbeO_ConnectorBehaviorContainerSub);

// Forward declarations
class kbeO_ConnectorBehaviorContainerIT;

// Class definition

class kbeO_ConnectorBehaviorContainer  : public mem_AllocationOperators
{
    friend class kbeO_ConnectorBehaviorContainerIT;
    // friend class kbeO_ApiExtension;
public:

    kbeO_ConnectorBehaviorContainer();
    const kbeO_ConnectorBehaviorOption& operator[](const odb_String&) const;
    kbeO_ConnectorBehaviorOption& get(const odb_String&);
    const kbeO_ConnectorBehaviorOption& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kbeO_ConnectorBehaviorContainerSub container;
};

class kbeO_ConnectorBehaviorContainerIT  : public mem_AllocationOperators
{
public:
    kbeO_ConnectorBehaviorContainerIT(const kbeO_ConnectorBehaviorContainer& mc);

    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const kbeO_ConnectorBehaviorOption& currentValue() const;
    
private:
    kbeO_ConnectorBehaviorContainerSubIT iter;
};

#endif // kbeO_ConnectorBehaviorContainer_h
