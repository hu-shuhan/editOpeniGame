//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasAtiCondVar_h
#define SMABasAtiCondVar_h

// Begin local includes
#include <mem_AllocationOperators.h>
// End local includes

class SMABasAtiThreadFactory;
class SMABasAtiDefaultThreadFact;

class SMABasAtiCondVar: public mem_AllocationOperators
{
 public:
   virtual ~SMABasAtiCondVar() { };

   virtual void Lock() = 0;
   virtual void Unlock() = 0;
   virtual void WaitForSignal(const int timeout = 0) = 0;
   virtual void Signal() = 0;
   virtual void Flag() = 0;

   /// Functions to identify these object (for debugging)
   virtual int id() const = 0;

   /// The following factories are allowed to produce condition variables
   friend class SMABasAtiThreadingModel;
   friend class SMABasAtiThreadFactory;
   friend class SMABasAtiDefaultThreadFact;

 protected:

   /// Constructor inaccessible on purpose; 
   /// Use SMABasAtiThreadingModel::newCondVar() to obtain condition variables.

   inline SMABasAtiCondVar() { }  

};

#endif
