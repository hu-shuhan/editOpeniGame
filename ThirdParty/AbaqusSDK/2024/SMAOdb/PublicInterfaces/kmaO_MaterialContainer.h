//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kmaO_MaterialContainer_h
#define kmaO_MaterialContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kmaO_MaterialContainer.h
//
// Description:  Material Container object on kmaO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <kmaO_Material.h>
#include <cow_Btree.h>

COW_BTREE_CMP_DECL(odb_String, kmaO_MaterialCOW, kmaO_MaterialContainerSub);

// Forward declarations
class kmaO_MaterialContainerIT;

// Class definition

class kmaO_MaterialContainer  : public mem_AllocationOperators
{
    friend class kmaO_MaterialContainerIT;
    friend class kmaO_ApiExtension;
public:

    kmaO_MaterialContainer();
    const kmaO_Material& operator[](const odb_String&) const;
    kmaO_Material& get(const odb_String&);
    const kmaO_Material& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kmaO_MaterialContainerSub container;
};

class kmaO_MaterialContainerIT  : public mem_AllocationOperators
{
public:
    kmaO_MaterialContainerIT(const kmaO_MaterialContainer& mc);
    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const kmaO_Material& currentValue() const;
    
private:
    kmaO_MaterialContainerSubIT iter;
};

#endif // kmaO_MaterialConatiner_h
