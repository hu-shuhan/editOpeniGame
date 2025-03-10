//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef omi_Singleton_h
#define omi_Singleton_h

// This file defines a class template, which provides a uniform interface
// to enforce singleton behavior for its instance classes. All public
// access to a singleton class must invoke the static Instance method
// provided by this template. For example:
// 
//    fac_Class::Instance().Methods(args);
// 
// This singleton form implicitly constructs the singleton object on
// demand. It requires a default constructor. If the class requires
// specific arguments for construction or it must be constructed at a
// specific point during execution, use the omi_SingletonExplicit
// template. The omi_Singleton form is preferred; use the more
// complicated form only when necessary.
//
// A singleton class must be declared as follows. The class must inherit
// from its singleton instance. The class must provide a default
// constructor. This must be declared private, it must be the only
// constructor, and the instance class must be declared a friend, so that
// only this template can construct an object. For example:
// 
//    class fac_Class : public omi_Singleton<fac_Class> {
//        friend class omi_Singleton<fac_Class>;
//        fac_Class();
// 
// A singleton class must explicitly instantiate it singleton interface
// by including its class header file and this template's implementation
// file, followed by an explicit instantiator. For example:
// 
//    #include <fac_Class.h>
//    #include <omi_Singleton.T>
//    OMI_SINGLETON(fac_Class);
//
// A singleton class must be finalized within the fac_initialize.txt file
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

// Begin local includes
#include <omi_SingletonBase.h>
// End local includes

template <class Class>
class omi_Singleton : public omi_SingletonBase<Class> {
public:

    // This Instance method constructs an object on demand. It must be
    // out-of-line to avoid the need to export the static data member.

    static Class& Instance();
};

#endif /* omi_Singleton_h */
