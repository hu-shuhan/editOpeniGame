//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_AgentListObj_h
#define cls_AgentListObj_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//   
// Modification history:
//      Rob deFriesse 10/99 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes
#include <mem_AllocationOperators.h>

#include <typ_typeTag.h>

#include <cls_IntCOW.h>

// End local includes

class cls_Uid;
class cls_WriteVisitor;
class cls_MementoWrite;
class cls_PC;

class cls_AgentListObj  : public mem_AllocationOperators
{
public:
  virtual ~cls_AgentListObj();

  virtual int Size(void) = 0;

  virtual const cls_IntCOW* Get(int index, typ_typeTag baseCid) = 0;

  virtual int Snap(void) = 0;
  virtual void Flush(void) = 0;

  virtual int DoWrite(int index) = 0;
  virtual void SetDirty(int index) = 0;

  virtual cls_MementoWrite* MakeWriteObj(int index, typ_typeTag cid, const cls_Uid& uid) = 0;
  virtual cls_MementoWrite* MakeWriteObjWithPtr(int index, typ_typeTag cid, const cls_Uid& uid, const void* objPtr);

  virtual void RefCountInc(void) = 0;
  virtual void RefCountDec(void) = 0;
  virtual int  GetRefCount(void) const = 0;

  virtual void Release(void) const = 0;

  virtual int ReadOnly(void) const = 0;

  virtual void RegisterPC(cls_PC* pc) const = 0;

private:
};

#endif
