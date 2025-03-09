//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_GlobalSingleton_h
#define thr_GlobalSingleton_h

// This file defines a class template which provides a uniform interface
// to enforce cross-thread singleton behavior for its instance classes. 
// This means that a single singleton will exist across all threads. 
// Lock and Unlock methods are provided as part of the class to provide
// an easy way to protect data and code. Construction and finalization 
// are thread-safe, so the user need not worry about locking constructor
// or destructor code. See thr/test/impl/thrT_GS_Test.C for usaage examples.

// All public access to a singleton class must invoke the static Instance 
// method provided by this template. For example:
// 
//    fac_Class::Instance().Methods(args);
// 
// This singleton form implicitly constructs the singleton object on
// demand. It requires a default constructor. There is no thread-safe
// equivalent of the omi explicit singleton.
//
// A singleton class must be declared as follows. The class must inherit
// from its singleton instance. The class must provide a default
// constructor. This must be declared private, it must be the only
// constructor, and the instance class must be declared a friend, so that
// only this template can construct an object. For example:
// 
//    class fac_Class : public thr_GlobalSingleton<fac_Class> {
//        friend class thr_GlobalSingleton<fac_Class>;
//        fac_Class();
// 
// A singleton class must explicitly instantiate it singleton interface
// by including its class header file and this template's implementation
// file, followed by an explicit instantiator. For example:
// 
//    #include <fac_Class.h>
//    #include <thr_GlobalSingleton.T>
//    THR_GLOBALSINGLETON(fac_Class);
//
// A singleton class must be finalized within the fac_initialize.C file
// of its facility by calling the static Finalize method provided by this
// template. For example:
// 
//    #include <fac_Class.h>
//    /// End of Headers ///
// 
//    /// Begin Facility Finalization ///
//    fac_Class::Finalize();
//    /// End Facility Finalization ///
// 
// 

// ------------------------------ IMPORTANT ------------------------------
// It is assumed that the Finalization of the global singleton will be done
// at the end of a run. No effort is made to protect against Finalization 
// of the singleton by one thread when another thread may still be using
// an instance of the singleton. In order to protect against this, the 
// finalization should be done in fac_finalize, and all threads should be
// done working at that time.


// Begin local includes
#include <thr_GlobalSingletonBase.h>
// End local includes

class thr_Mutex;

template <class Class>
class thr_GlobalSingleton : public thr_GlobalSingletonBase<Class> {
public:
    // This Instance method constructs an object on demand. It must be
    // out-of-line to avoid the need to export the static data member.
    //
    static Class& Instance();
    
    void Lock()   { thr_GlobalSingletonBase<Class>::LockBase(); }
    void Unlock() { thr_GlobalSingletonBase<Class>::UnlockBase(); }
};


#endif // thr_GlobalSingleton_h

