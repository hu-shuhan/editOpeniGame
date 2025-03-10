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
// File Name: omu_DictIntObj.h
// 
// Description:
//    Defines the callable interface on the dictionaries.
//


#ifndef omu_DictIntObj_h
#define omu_DictIntObj_h

//
// Begin Local Includes
//
#include <omi_types.h>
#include <omu_InterfaceObj.h>

//
// Forward Declarations
//

class omu_Primitive;
class omu_Arguments;
class omu_PrimVisitor;
class omu_PrimList;
class omu_PrimNumber;

//
// Class definition
//

class omu_DictIntObj : public omu_InterfaceObj
{
  public:
    //
    // Describe the Type to the omu_Interface.
    // This TypeName should be used for all DictIntObj's
    // ie derived classes should NOT call DescribeType
    // in their constructor.
    static const char * TypeName() { return "Repository"; }

#ifndef MUST_USE_SHORTCUT    
    omu_DictIntObj() // deprecated to add shortcut
	: omu_InterfaceObj(omu_PythonRules, omu_Dictionary)
    {}
#endif   
    omu_DictIntObj(const omu_Shortcut& sc)
	: omu_InterfaceObj(sc,omu_PythonRules, omu_Dictionary){};

    virtual ~omu_DictIntObj();
};

//
// Class definition
//

template <class T>
class omu_DictIntObjABC : public omu_DictIntObj
{
  public:
    virtual ~omu_DictIntObjABC();

    //
    // Mapping (dictionary) Interface
    //
    virtual
    int Length() const = 0;	// Get the length of the map

    
    virtual
    omu_Primitive *Subscript(	// get a value based on the key
	const T &key		// i.e dict['key'], dict[34], dict[FRONT]
	) const = 0;

    // Get the printable version of the key
    cow_String GetKeyAsString(
	const T &key		
	) const;

    virtual
    int Assign(			// assign to a key in the dictionary
	const T &key,			// i.e. dictionary['key']=5
	omu_Primitive *value
	)=0;

    virtual
    int Remove(			// remove a key
	const T &key		// i.e. del dictionary['key']
	)=0;

    virtual
    bool ProcessDeltaQuery() const;

    //
    // Methods keys/values -  virtuals need to be redefined in derived dicts
    //
    virtual omu_PrimList *  Values() const; 
    virtual cow_List<T>     Keys() const = 0;
    virtual bool            HasKey(const T &key) const;
    virtual omu_PrimList *  Items() const ;
    virtual omu_Primitive * ChangeKey(const T &oldKey,const T &newKey) ;
    virtual omu_Primitive * Summary() const ;
    virtual omu_Primitive * SummaryRow(const T &key,int& index) const ;

    omu_PrimList *          MyKeys() const;
    omu_PrimList *          MyVals() const { return this->Values();}
    omu_PrimList *          MyItems() const { return this->Items();}
    omu_PrimNumber *        MyHasKey(omu_Arguments& args);
    omu_Primitive *         MyChangeKey(omu_Arguments& args);
    omu_Primitive *         MySummary() const { return this->Summary();}
    omu_Primitive *         MySummaryRow(const T &key,int& idx) const 
			    { return this->SummaryRow(key,idx);}

    //
    // Accept method. The pre-instantiated instances will define this
    //
    virtual void Accept(omu_PrimVisitor *);
    
    //
    // omu_Primitive interface
    //
    virtual cow_String AsString(int depth=0) const;

    //
    // CommandPath is used to get path(s) for queries (omu_CmdHistory)
    //    Return an empty string for methods that do not change the model
    //
    cow_String CommandPath(const cow_String &path, const cow_String &method, omu_Arguments &args) const;


  protected:
#ifndef MUST_USE_SHORTCUT    
    omu_DictIntObjABC(); // deprecated to add shortcut
#endif
    
    omu_DictIntObjABC(const omu_Shortcut& sc);
};


#endif  // #ifdef omu_DictIntObj_h
