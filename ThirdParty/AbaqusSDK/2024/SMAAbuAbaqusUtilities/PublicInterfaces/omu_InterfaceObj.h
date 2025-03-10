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
// File Name: omu_InterfaceObj.h
// 
// Description:
//     Interface object. An abstract base class for adapter objects which
//     provide an  "interpreted" interface to a class.
//     
//     A developer would create a concrete interface object to bind an
//     object to the query and command language.
//
//     The reason for the base class is to allow the qry and pyt roles
//     to treat all interface objects uniformly.
//


#ifndef omu_InterfaceObj_h
#define omu_InterfaceObj_h

//
// Begin Local Includes
//
#include <mem_AllocationOperators.h>
#include <mem_AllocationOperatorsTiny.h>
#include <omi_types.h>
#include <omu_Primitive.h>
#include <omu_Shortcut.h>
#include <cow_String.h>
#include <cow_ListString.h>

//
// Forward Declarations
//
class omu_Arguments;
class omu_PrimWrapper;

//
// Class definition
//

enum omu_PythonType
{
    omu_Interface,
    omu_Sequence,
    omu_Dictionary
};

enum omu_OperationRules
{
    omu_Numeric,
    omu_PythonRules
};

class omu_InterfaceObj : public omu_Primitive
{
  public:
    //
    // Typedefs for methods, and GetMember functions
    //
    typedef omu_Primitive *(omu_InterfaceObj::*methodFunc)(omu_Arguments &) const;
    typedef omu_Primitive *(omu_InterfaceObj::*getmemberFunc)(void) const;
    typedef omu_Primitive *(omu_InterfaceObj::*setmemberFunc)(omu_Arguments &) const;

    //
    // Typedef for type descriptor, member table, method table
    //    these tables should be statics in the derivatives of this class
    struct methodTable
    {
        const char *methodName;
        methodFunc method;
    };
    struct memberTable
    {
        const char *memberName;
        getmemberFunc getfunc;
        setmemberFunc setfunc;
    };

    // Special tables for methods/members that are currently deprecated
    struct deprecatedMethodTable
    {
        const char *methodName;
        methodFunc method;
        const char *deprecatedAfter; // i.e. "6.1"
        const char *message; // i.e. "Replaced by xxx.GetValue()"
    };
    struct deprecatedMemberTable
    {
        const char *memberName;
        getmemberFunc getfunc;
        setmemberFunc setfunc;
        const char *deprecatedAfter; // i.e. "6.1"
        const char *message; // i.e. "Replaced by xxx.sections"
    };

    // Destructor
    virtual ~omu_InterfaceObj();

    // Define keyword names
    static omn_FixedString  objectToCopyCmd();

    //
    // Describe the Type to the omu_Interface.
    //
    // Note: The Type Name of the derived class (the name of the Python Type)
    //       should be returned by a static method of the derived class 
    //       named TypeName().
    //       TypeName() should then be used as the first argument
    //       to DescribeType.
    //
    static const char * TypeName() { return "AbaqusObject"; }

    // The exposed type name is found by comparing the names in the
    // type hierarchy with those in the type registry
    cow_String ExposedTypeName() const;
    
    void
    DescribeType(const char *,
                 const methodTable [] = 0,
                 const memberTable [] = 0,
                 const deprecatedMethodTable [] = 0,
                 const deprecatedMemberTable [] = 0);

    //
    // Member and Method interface.
    //
    virtual
    bool IsMethod(const char*,
                  bool &deprecatedFound) const;

    virtual bool ShouldJournal(const cow_String &method) const;
    // This method is called by the python binding to determine if a 
    // method should be journaled. This method should not be called
    // by derived interface objects.

    virtual
    omu_Primitive *CallMethod(
        const char *path,
        const char *methodName,
        omu_Arguments &args,
        bool &deprecatedFound
        ) const;

    virtual
    omu_Primitive *GetMember(const char *memberName,
                             bool &deprecatedFound) const;

    virtual
    omu_Primitive *SetMember(
        const char *memberName,
        omu_Arguments &args,
        bool &deprecatedFound
        ) const;

    virtual
    void DeleteMember(const cow_String &memberName) const;
    // Some classes support the ability to remove a data member
    // by calling del. (e.g. del object.member) for these types
    // of objects, this virtual should be implemented.
    // This function should throw exceptions in the following conditions
    //
    //   delete of attribute is
    //   not supported for this   ==>  AttributeError: read only atttributes
    //   (or any) attributes
    //
    //   delete is supported but
    //   the attribute given does ==>  AttributeError: <attrname>
    //   not exist
    // see omu_AttributeErrorException


    //
    // return list with names of member/method functionsa
    //
    virtual cow_ListString
    MemberList() const;

	// This method can be re-implemented to return a member list to be used during qry eval
	// It can be used to bypass time-expensive members which could then be implemented on
	// a need-to-know basis using a member proxy.

	// The default implementation get the original member list and remove any members found in
	// the MemberExcludeListForQry.
    virtual cow_ListString
    MemberListForQry() const;

	//  This method can be implement to return a list of members to be bypassed during qry eval.
	//  The default implementation is to return an empty list
    virtual cow_ListString
    MemberExcludeListForQry() const;

    virtual cow_ListString
    MethodList() const;

    //
    // Instance Type
    //
    bool IsA(const char *type) const;

    //
    // Numeric type access methods
    //
    omu_PythonType      PythonType() { return pythonType; }
    omu_OperationRules  OperationRules() { return operationRules; }

    // Primitive interface
    //
    // Used by Python str(object)
    virtual cow_String AsString(int depth=0) const;

    // Used by Python repr(object)
    virtual cow_String AsRepr() const;

    // The FullRepr method should be implemented in a derived class
    // if the object should accessed in only one particular way, and
    // the functionality cannot be implemented using a shortcut.
    // eg. the path to the XYPlot object should always be
    //            session.xyPlots[xyPlotName]
    //     even when it can be accessed in multiple ways, as shown
    //            xyp1 = session.XYPlot(xyPlotName)
    //            xyp2 = session.viewports[vpName].displayedObject
    // A shortcut provides similar functionality but not within the object.
    // This should also be implemented if the object repr changes during
    // the life of the object.
    virtual cow_String FullRepr() const;

    // The default is of the form <XxxXxxx object[ named 'abcde']>
    cow_String DefaultRepr() const;

    virtual bool operator==(const omu_Primitive &) const;

    // TypeString should not be implemented by derived InterfaceObjects. 
    // The base method will return the correct value for TypeString as long
    // as the static TypeName method is implemented by the derived class.
    cow_String TypeString() const; // used in TypeErrorExceptions.
    cow_String BaseType() const;
    
    // list of base type names starting at BaseType, not incl TypeString
    cow_ListString BaseTypes() const;

    virtual cow_String JournalVariable() const; // used for Temporaries

    // Get the shortcut for the object.
    // If not implemented in derived class, return base class shortcut.
    // The default shortcut returns empty string for path, but many
    // derived classes pass a shortcut to the base class omu_InterfaceObj.
    // Derived classes contain shortcut members of different types, 
    //   hence cannot use the Shortcut method of the derived class.
    virtual const omu_Shortcut &BaseShortcut() const;
    
    // BaseShortcutGetABC - Get Abstract Base Class of omu_Shortcut.
    // Interface objects that contain ddr_Shortcuts define BaseShortcutGetABC
    // because they are derived from omu_ShortcutBase, not omu_Shortcut.
    // The default implementation returns BaseShortcut().
    virtual const omu_ShortcutBase &BaseShortcutGetABC() const;
    
    // virtual method to allow derived interface objects to modify
    // command path used to trigger queries
    virtual cow_String CommandPath(
        const cow_String &, 
        const cow_String &,
        omu_Arguments &args
        ) const;
    //
    // number methods on omu_InterfaceObj
    //
    virtual omu_InterfaceObj *Coerce(omu_Arguments &) const;
    virtual omu_InterfaceObj *Cast(omu_Arguments &) const; //explicit cast method
    virtual omu_Primitive *Absolute() const;

    virtual omu_Primitive *AddOperation(omu_InterfaceObj *) const;
    virtual omu_Primitive *SubOperation(omu_InterfaceObj *) const;
    virtual omu_Primitive *MulOperation(omu_InterfaceObj *) const;
    virtual omu_Primitive *DivOperation(omu_InterfaceObj *) const;
    virtual omu_Primitive *UnaryNegative() const;

    // The default implementation of this method returns true
    // Reimplement this method in the dervied class if the number object
    // can take a value of 0.
    // eg. an xyData object can never be 0 since a zero-length xyData
    //     is forbidden. A fieldData object can be 0 if all the associated
    //     values in the fieldData object are 0.
    virtual int NonZero();

    //
    // Visitor Accept method
    //
    virtual void Accept(omu_PrimVisitor *);

    //
    // Create an object wrapper for whole object queries
    // This method should only be called by the prim to obj visitor 
    // during query processing.  The memory is handed off to the
    // caller.  The caller should delete the wrapper when done.
    //
    virtual
    omu_PrimWrapper *ConstGetWrapper() const;

    // implement weak reference (stale handle to object)
    virtual bool IsStale() const;
    virtual void SetStale(bool value);
    virtual int Compare(const omu_InterfaceObj *obj) const;
    virtual int ComparePath(const cow_String &path) const;
    
  protected:
#ifndef MUST_USE_SHORTCUT
    omu_InterfaceObj(); // deprecated to add shortcut
    
    omu_InterfaceObj( omu_OperationRules, omu_PythonType type = omu_Interface ); // deprecated to add shortcut
#endif
    
    omu_InterfaceObj(const omu_Shortcut&);
    omu_InterfaceObj(const omu_Shortcut&, omu_OperationRules, 
                     omu_PythonType type = omu_Interface );

    struct TypeDesc  : public mem_AllocationOperators
    {
        TypeDesc()
          : className(""), methods(0), members(0), dmethods(0), dmembers(0) {};
        TypeDesc(const char *name,
                 const methodTable *mtable, 
                 const memberTable *mbers,
                 const deprecatedMethodTable *dmtable, 
                 const deprecatedMemberTable *dmbers)
          : className(name), methods(mtable), members(mbers),
                             dmethods(dmtable), dmembers(dmbers) {}
        cow_String className;
        const methodTable *methods;
        const memberTable *members;
        const deprecatedMethodTable *dmethods;
        const deprecatedMemberTable *dmembers;
    };

    // Journaling Control functions 

    void DoNotJournal(const cow_String &method);
    // Interface objects that wish to
    // prevent journaling certain methods to the MDB journal file
    // (.jnl) should call this interface with the names of each method
    // that should not be journaled.
    // e.g.
    // MyIntObj::MyIntObj()
    // {
    //    DescribeType(...);
    //    DoNotJournal("foo");
    // }
    // The foo method of the MyIntObj will not be written to the 
    // MDB journal file.



    //
    // The inheritance hierarchy of the instance of the interface object
    // is described by this list of type object. There is a hard 
    // coded array to avoid the overhead of a list class, since we 
    // don't expect the hierarchy to be very deep.
    //
    // The type descriptions are kept in base class first order. element
    // 0 contains the most base class, element hierarchyDepth-1 holds the
    // most derived type description, and the actual type of this class.
    //
    TypeDesc typeHierarchy[10];
    int hierarchyDepth;

    // members indicating operator precedence. Used when implementing
    // numeric operations on interface objects.
    omu_OperationRules  operationRules;
    omu_PythonType      pythonType;

    // Objects such as Material have members that do not always
    //    exist.  In these cases, the following flag should be set.
    bool checkForDynamicMembers;

    // Default empty method and member tables used by DefineType
    static methodTable emptyMethods [];
    static memberTable emptyMembers [];
    static deprecatedMethodTable emptyDMethods [];
    static deprecatedMemberTable emptyDMembers [];

  private:
#ifdef MUST_USE_SHORTCUT
    omu_InterfaceObj(); // _not_ deprecated for add shortcut
#endif
    
    // The method names that appear in this list should not be journaled
    // to the MDB journal file.
    cow_ListString doNotJournalList;
    
    omu_Shortcut  shortcut;
    bool isStale; // set this when underlying object is deleted
};


#endif  // #ifdef omu_InterfaceObj_h
