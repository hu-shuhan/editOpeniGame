//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddr_UserData_h
#define ddr_UserData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class ddr_UserData  : public mem_AllocationOperators
{
public:
     ddr_UserData();
     virtual ~ddr_UserData();

     virtual ddr_UserData* Copy() const = 0;

     virtual typ_typeTag DynTypeId() const;
     static typ_typeTag TypeId();

     ddr_UserData(const cls_ReadVisitor& rv);
     static void* Ctor(cls_ReadVisitor& rv);
     virtual void DBWrite(const cls_WriteVisitor& wv) const = 0;
     static void InitializeMetadata(cls_ClassRegistrar& reg);

private:
     cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(ddr_UserData, cow_Virtual);

#endif
