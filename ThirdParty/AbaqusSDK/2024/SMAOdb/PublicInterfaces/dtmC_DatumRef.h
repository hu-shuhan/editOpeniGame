//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef dtmC_DatumRef_h
#define dtmC_DatumRef_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: dtmC_DatumRef.h
//
// Description:  Definition of datum reference class.  This object is used by
//               EOs to retrieve datums.
//


// Includes

// Begin local includes
#include <mem_AllocationOperators.h>
#include <typ_typeTag.h>
#include <cow_COW.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <cls_xpListString.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;


// Class definition

class dtmC_DatumRef : public mem_AllocationOperators
{
  public:

    dtmC_DatumRef();
    dtmC_DatumRef(int datumId, int instId=0, const cow_String& instName = "", int axisId=0);

    bool IsNullRef() const;

    int DatumId()const {return _datumId;}
    int& DatumId() {return _datumId;}

    int InstanceId()const {return _instanceId;}
    int& InstanceId() {return _instanceId;}

    int AxisId()const {return _axisId;}
    int& AxisId() {return _axisId;}

    const cow_ListString OwnerNames() const { return _ownerNames; }

    // Clone function
    dtmC_DatumRef* Copy() const;

    bool operator==(const dtmC_DatumRef&) const;
    bool operator!=(const dtmC_DatumRef&) const;

    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    // Database interface
    dtmC_DatumRef(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

private:
    cls_xpListString _ownerNames;
    int _datumId;
    int _instanceId;
    int _axisId;

    cls_Uid m_ClsUid;


    friend class dtmK_Datum;

};

COW_COW2_DECL(dtmC_DatumRef,cow_Virtual);


#endif /* dtmC_DatumRef_h */
