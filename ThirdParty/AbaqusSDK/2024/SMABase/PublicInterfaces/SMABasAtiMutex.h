//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasAtiMutex_h
#define SMABasAtiMutex_h

// Begin local includes
#include <mem_AllocationOperators.h>
// End local includes

class SMABasAtiThreadFactory;
class SMABasAtiDefaultThreadFact;

class SMABasAtiMutex: public mem_AllocationOperators
{
 public:
   virtual ~SMABasAtiMutex() { };

   virtual void Lock()  = 0 ;
   virtual void Unlock() = 0;
   virtual int  TryLock() = 0; //! returns 1 if lock is acquired, 0 otherwise

   /// Functions to identify these object (for debugging)
   virtual int id() const = 0;

   /// The following factories are allowed to produce mutexes
   friend class SMABasAtiThreadingModel;
   friend class SMABasAtiThreadFactory;
   friend class SMABasAtiDefaultThreadFact;

 protected:

   /// Constructor inaccessible on purpose; 
   /// Use SMABasAtiThreadingModel::newMutex() to obtain mutexes.

   inline SMABasAtiMutex() { }  

};

#endif
