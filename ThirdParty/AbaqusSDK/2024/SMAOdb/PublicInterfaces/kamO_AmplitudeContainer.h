//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef kamO_AmplitudeContainer_h
#define kamO_AmplitudeContainer_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: kamO_AmplitudeContainer.h
//
// Description:  Amplitude Container object on kamO
//
// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <odb_String.h>
#include <odb_Amplitude.h>
#include <cow_Btree.h>

COW_BTREE_CMP_DECL(odb_String, odb_AmplitudeCOW, kamO_AmplitudeContainerSub);

// Forward declarations
class kamO_AmplitudeContainerIT;

// Class definition

class kamO_AmplitudeContainer  : public mem_AllocationOperators
{
    friend class kamO_AmplitudeContainerIT;
    friend class kamO_ApiExtension;
public:

    kamO_AmplitudeContainer();
    const odb_Amplitude& operator[](const odb_String&) const;
    odb_Amplitude& get(const odb_String&);
    const odb_Amplitude& constGet( const odb_String& ) const;
    int size() const;
    bool isEmpty() const;
    bool isMember(const odb_String&) const;
    
private:
    kamO_AmplitudeContainerSub container;
};

class kamO_AmplitudeContainerIT  : public mem_AllocationOperators
{
public:
    kamO_AmplitudeContainerIT(const kamO_AmplitudeContainer& mc);

    
    void first();
    void next();
    bool isDone();
    
    const odb_String& currentKey() const;
    const odb_Amplitude& currentValue() const;
    
private:
    kamO_AmplitudeContainerSubIT iter;
};

#endif // kamO_AmplitudeConatiner_h
