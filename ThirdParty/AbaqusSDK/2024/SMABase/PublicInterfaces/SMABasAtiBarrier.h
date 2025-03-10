//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasAtiBarrier_h
#define SMABasAtiBarrier_h

// Begin local includes
#include <mem_AllocationOperators.h>
// End local includes

/// Abstract Base Class ( Interface Class) for a family of concrete classes,
/// each supporting a specific mode of multithreading.

class SMABasAtiBarrier: public mem_AllocationOperators
{
 public:

   virtual ~SMABasAtiBarrier() {};

   virtual void BarrierSync(const unsigned int gangSize) = 0;

   virtual int id() const { return 301; } 

protected:

   SMABasAtiBarrier() {} /// Inaccessible on purpose. To obtain barriers, use
                         /// SMABasAtiThreadingModel::newBarrier().
private:
    /// Copy and Assignment are not supported
    SMABasAtiBarrier(const SMABasAtiBarrier& source);
    SMABasAtiBarrier& operator=(const SMABasAtiBarrier& source);

};

#endif
