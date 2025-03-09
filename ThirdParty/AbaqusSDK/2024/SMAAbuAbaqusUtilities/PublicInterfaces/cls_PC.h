//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cls_PC_h
#define cls_PC_h

///////////////////////////////////////////////////////////////////////////////
//
//
// Description:
//
//
// Modification history:
//      Rob deFriesse 1/2001 original
//
////////////////////////////////////////////////////////////////////////// */

// Begin local includes
#include <mem_AllocationOperators.h>


// End local includes

class cls_PC  : public mem_AllocationOperators
{
public:
  cls_PC();
  virtual ~cls_PC();

  virtual void Flush() const;
  virtual void Detach();

  void Link(cls_PC* pc);

  inline cls_PC* Next() const { return m_Next; };
  inline cls_PC* Last() const { return m_Last; };

  void Check();
  void Display();

protected:
  void Reset();
  void Unlink();

private:
  cls_PC* m_Last;
  cls_PC* m_Next;
};

#endif
