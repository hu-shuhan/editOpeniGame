//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: omu_Shortcut.h
// 
// Description:
//    
//  virtual base class for bas shortcuts.  Contains the interface that
//  is not dependant on the type of shortcut or having an mdb


#ifndef omu_Shortcut_h
#define omu_Shortcut_h

//
// Begin Local Includes
//
#include <cow_String.h>
#include <cow_COW.h>
//
// Forward Declarations
//

class omu_ShortcutImpl;

COW_COW2_DECL(omu_ShortcutImpl,cow_Virtual);
//
// Forward Declarations
//

class omu_ShortcutImpl  : public mem_AllocationOperators
{
  public:
    virtual ~omu_ShortcutImpl();

    // Return the name of the object
    virtual cow_String Name() const = 0;

    // Return the path of the object
    virtual cow_String Path() const = 0;

    // Return the path of the object in Python API
    // - usually same as Path, but not in ddr_Shortcut
    virtual cow_String PythonPath() const;

    // true if the object is in a model
    virtual bool InModel() const = 0;

    // true if the object is in an odb
    virtual bool InOdb() const = 0;

    // true if the object is in the session
    virtual bool InSession() const = 0;

    virtual omu_ShortcutImpl* Copy() const = 0;


};


// Abstract base class for omu_Shortcut

class omu_ShortcutBase  : public mem_AllocationOperators
{
public:
    omu_ShortcutBase() {}

    virtual ~omu_ShortcutBase() {}

    virtual bool HasValue() const = 0;

    virtual cow_String Name() const = 0;

    virtual cow_String Path() const = 0;
    
    virtual cow_String PythonPath() const = 0;

};

class omu_Shortcut : public omu_ShortcutBase
{
public:

    // Constructors
    omu_Shortcut();

    omu_Shortcut(const  omu_ShortcutImpl&implSC);

    virtual ~omu_Shortcut();

    virtual bool HasValue() const;

    // Return the name of the object referred to by this shortcut.
    // The name is either:
    //    1. The name of the member in the parent object
    //    2. The name of the object in the repository.
    //    3. The index of the object in a sequence.
    virtual cow_String Name() const;

    // Return the path to the object referred to by this shortcut
    // The path is a concatenation of name of all the objects in the
    // parent-child relations that make up this shortcut.
    virtual cow_String Path() const;
    
    virtual cow_String PythonPath() const;

    // true if the object is in a model
    // false if the object is not a model object.
    virtual bool InModel() const;

    virtual bool InOdb() const;

    virtual bool InSession() const;

    // equality. Two shortcuts are equal if they have the same path
    bool operator==(const omu_Shortcut& rhs) const;

    bool operator!=(const omu_Shortcut& rhs) const {
        return ! operator==(rhs); }

protected:

    cow_COW<omu_ShortcutImpl, cow_Virtual<omu_ShortcutImpl> >  impl;    

};


#endif  // #ifdef omu_Shortcut_h
