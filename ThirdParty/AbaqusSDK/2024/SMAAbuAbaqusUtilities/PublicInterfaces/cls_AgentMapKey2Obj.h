//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_AgentMapKey2Obj_h
#define cls_AgentMapKey2Obj_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//   
// Modification history:
//      Rob deFriesse 8/99 original
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

template <class K> class cls_AgentMapKey2Obj  : public mem_AllocationOperators
{
public:
  virtual ~cls_AgentMapKey2Obj();

  virtual bool Snap(void) = 0;
  virtual void Flush(void) = 0;

  virtual void First() = 0;
  virtual void Next() = 0;
  virtual bool IsDone() const = 0;
  virtual const K& Key() const = 0;

  virtual const cls_IntCOW* Get(const K& key, typ_typeTag baseCid) = 0;

  virtual bool DoWrite(const K& key) const = 0;
  virtual bool Present(const K& key) const = 0;
  virtual void SetDirty(const K& key) = 0;

  virtual cls_MementoWrite* MakeWriteObj(const K& key, typ_typeTag cid, const cls_Uid& uid) = 0;
  virtual cls_MementoWrite* MakeWriteObjWithPtr(const K& key, typ_typeTag cid, const cls_Uid& uid, const void* objPtr);
  virtual void Remove(const K& key) = 0;

  virtual void RefCountInc(void) = 0;
  virtual void RefCountDec(void) = 0;
  virtual int  GetRefCount(void) const = 0;

  virtual void Release(void) const = 0;

  virtual bool ReadOnly(void) const = 0;

  virtual void RegisterPC(cls_PC* pc) const = 0;

private:
};

#endif
