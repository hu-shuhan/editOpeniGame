//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef kmaC_MaterialOption_h
#define kmaC_MaterialOption_h
///////////////////////////////////////////////////////////////////////////////
//
// File Name: kmaC_MaterialOption
//
// Description: Engineering object class for kmaC_MaterialOption
//

// Includes

// Begin local includes
#include <mem_AllocationOperators.h>

#include <cow_ArrayCOW.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <idb_Utils.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

//

class kmaC_MaterialOption  : public mem_AllocationOperators
{
  public:

    kmaC_MaterialOption();
    virtual ~kmaC_MaterialOption();

    virtual kmaC_MaterialOption* Copy() const = 0;

    virtual void AssertValid() const = 0;      // Consistency checking

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

    kmaC_MaterialOption *             DeepCopy() const {
        return (kmaC_MaterialOption*)DDB_DeepCopyPtr(this, DynTypeId());
    }

    bool                    IsA(typ_typeTag) const;

public: // Database interface
    kmaC_MaterialOption(const cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(kmaC_MaterialOption, cow_Virtual);

#endif
