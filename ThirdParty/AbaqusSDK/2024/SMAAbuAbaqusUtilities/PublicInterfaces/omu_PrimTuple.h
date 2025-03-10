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
// File Name: omu_PrimTuple.h
// 
// Description:
//   A tuple object that is supposed to support the same interface as the
//   python tuple object.
//     
//

#ifndef omu_PrimTuple_h
#define omu_PrimTuple_h

//
// Begin Local Includes
//
#include <omu_DictIntObjInst.h>
#include <omu_PrimSequence.h>
#include <omu_PrimitiveCowList.h>

#include <cow_ListInt.h>
#include <cow_ListFloat.h>
#include <cow_ListString.h>
#include <cow_ListTranslatedString.h>
#include <cow_ListBool.h>
#include <cow_ListDouble.h>

#include <cow_2DArrayDouble.h>
#include <cow_2DArrayString.h>
#include <cow_MapString2Float.h>

class omu_InterfaceObj;
class omu_PrimExpr;
class omu_PrimEnumBase;
class omu_PrimNone;
class omu_PrimType;
class omu_PrimPath;
class omu_PrimArray;
class omu_PyObjectWrapper;
class cow_String;
class atr_String;
class omi_complex;
class SMABasUtlFilePath;
template <class T> class omu_PrimDictionary;
class omu_PyObjectWrapper;
class omu_ListPyObjectWrapper;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;
template <class T> class omu_DictPyObjectWrapper;
typedef omu_DictPyObjectWrapper<cow_String> omu_DictStrPyObjectWrapper;
typedef omu_DictPyObjectWrapper<int> omu_DictIntPyObjectWrapper;

//
// Class definition
//
class omu_PrimTuple : public omu_PrimSequence
{
  public:
    // Constructors, Destructor

    omu_PrimTuple(int size,char type='(');
    omu_PrimTuple(char type='(');
    omu_PrimTuple(omu_PrimitiveCowList, char type='(' );

    // Construct from cow lists
    omu_PrimTuple(const cow_ListInt&,char type='(');
    omu_PrimTuple(const cow_ListBool&,char type='(');
    omu_PrimTuple(const cow_ListFloat&,char type='(');
    omu_PrimTuple(const cow_ListString&,char type='(');
    omu_PrimTuple(const cow_ListTranslatedString&,char type='(');
    omu_PrimTuple(const cow_ListEnum&,char type='(');
    omu_PrimTuple(const cow_ListDouble&,char type='(');

    // Construct from C array
    omu_PrimTuple(const float *, const int, char type='(');
    omu_PrimTuple(const int *, const int, char type='(');

    // Construct from cow 2dArray
    omu_PrimTuple(const cow_2DArrayDouble&,char type='(');
    omu_PrimTuple(const cow_2DArrayString&,char type='(');

    // Construct from omu_MapString2Float
    omu_PrimTuple(const cow_MapString2Float&,char type='(');

    // Construct from omu_PrimArray
    omu_PrimTuple(const omu_PrimArray&,char type='(');

    // destructor
    ~omu_PrimTuple();

    //
    // Primitive interface
    //
    // Virtuals from the base class.
    //
    virtual cow_String AsString(int depth=0) const;
    virtual cow_String AsRepr() const;
    virtual bool operator==(const omu_Primitive &) const;
    virtual cow_String TypeString() const; // used in TypeErrorExceptions.
    
    //
    // Visitor Accept method
    //
    virtual void Accept(omu_PrimVisitor *);
    //  needed to support cow_List of omu primitive cows
    virtual omu_Primitive  *Copy() const;
    
    //
    // Sequence interface 
    //
    int Length() const {return size;};
    const omu_Primitive *Index(int i) const;        
    // Create an orphan Tuple  by slicing the tuple from
    // i to j.
    omu_PrimSequence *Slice(int i, int j) const; // orphan return.
    omu_PrimSequence *Repeat(int _num) const; 
    omu_PrimSequence *Concat(omu_PrimSequence *_newSeq) const;

    
    //
    // Tuple interface 
    //
    virtual void Put(schar );
    virtual void Put(short );
    virtual void Put(int );
    virtual void Put(bool );
    virtual void Put(uint );
    virtual void Put(const char *);
    virtual void Put(const cow_String &);
    virtual void Put(const atr_String &);
    virtual void Put(const omi_complex &);
    virtual void Put(const SMABasUtlFilePath &);
    virtual void Put(double );
    virtual void Put(const omu_PrimExpr &);
    virtual void Put(const omu_PrimPath &);   // copy a path (shallow copy)
    virtual void Put(const omu_PrimTuple &);  // copy a tuple (shallow copy)
    // copy a python array (shallow copy)
    virtual void Put(const omu_PrimArray &);  
    //copy a dictionary (shallow copy)
    virtual void Put(const omu_PrimDictionary<int>&);
    //copy a dictionary (shallow copy)
    virtual void Put(const omu_PrimDictionary<cow_String>&);
    virtual void Put(omu_InterfaceObj *); // borrow an interface pointer
    virtual void Put(omu_PyObjectWrapper *); // borrow an PyObject pointer
    virtual void Put(omu_Primitive *);    // adopt a primitive pointer
    virtual void Put(const omu_PrimEnumBase &);
    virtual void Put(const omu_PrimNone &);
    virtual void Put(const omu_PrimType &);
    virtual void Put(omu_DictStrPyObjectWrapper&);
    virtual void Put(omu_DictIntPyObjectWrapper&);
    virtual void Put(omu_ListPyObjectWrapper&);

    // Put for cow lists
    virtual void Put(const cow_ListInt&);
    virtual void Put(const cow_ListBool&);
    virtual void Put(const cow_ListFloat&);
    virtual void Put(const cow_ListString&);
    virtual void Put(const cow_ListTranslatedString&);
    virtual void Put(const cow_ListEnum&);
    virtual void Put(const cow_ListDouble&);

    // Put for cow 2dArray
    virtual void Put(const cow_2DArrayDouble&);
    virtual void Put(const cow_2DArrayString&);

    // Put for omu_MapString2Float
    virtual void Put(const cow_MapString2Float&);



    //
    // Get values by type.
    // With these simple types, the C++ compiler will do static type
    // checkin for the arguments, and choose the correct Get call
    // The Get call will then test to see if the cursor points to 
    // and object of that type if it does it will assign the reference 
    // with the value. If the it is not the correct type, it will
    // set the error flags, and NOT modify the reference.
    //
    // example call:
    // 
    //     int i = 5;   // default value for optional arguments;
    //     args.Begin();
    //     args.Get(i);
    //     args.End();
    //
    virtual void Get(bool &);
    virtual void Get(schar &);
    virtual void Get(short &);
    virtual void Get(int &);
    virtual void Get(uint &);
    virtual void Get(double &);
    virtual void Get(cow_String &);
    virtual void Get(atr_String &);
    virtual void Get(omi_complex &);
    virtual void Get(SMABasUtlFilePath &);
    virtual void Get(omu_PrimPath &);
    virtual void Get(omu_PrimTuple &);
    virtual void Get(omu_PrimDictionary<int> &);
    virtual void Get(omu_PrimDictionary<cow_String> &);
    virtual void Get(omu_PrimEnumBase &);
    virtual void Get(omu_PrimArray &);
    virtual void Get(omu_PrimNone &);
    virtual void Get(omu_PrimType &);
    virtual void Get(cow_ListInt &);
    virtual void Get(cow_ListBool &);
    virtual void Get(cow_ListFloat &);
    virtual void Get(cow_ListString &);
    virtual void Get(cow_ListTranslatedString &);
    virtual void Get(cow_ListDouble &);
    virtual void Get(cow_2DArrayDouble &);
    virtual void Get(cow_2DArrayString &);
    virtual void Get(cow_MapString2Float &);
    virtual void Get(omu_DictIntPyObjectWrapper&);
    virtual void Get(omu_DictStrPyObjectWrapper&);
    virtual void Get(omu_ListPyObjectWrapper&);
 
    //
    // For omu_InterfaceObj the types are more dynamic, it is not practical 
    // to put all concrete types in this interface, so the get interface
    // changes slightly. The Get() returns an interface object (allowing the
    // return value to be cast to the correct type) if the object at the 
    // cursor is an interface object that inherits from the type name given.
    // Otherwise the error state is set and the default is returned.
    //
    // As a matter of convention Interface Objects that are intended to be
    // used as arguments should define a static member:
    // static const char *TypeString()
    // which returns the string for the type, freeing the clients from knowing
    // the details of the name.
    //
    // example call:
    // 
    //     const FooIntObj *fooIO;
    //     args.Begin();
    //     fooIO = (const foo *)args.Get(FooIntObj::TypeString(),0);
    //     args.End();
    //
    // (Note: C++ does not allow a Derived object to be passed as a
    //        reference to a Base object. So the same style could not
    //        be used as above in the other Gets())
    //
    virtual const omu_InterfaceObj *Get(
        const char *typeName,
        const omu_InterfaceObj *def
        );

    virtual const omu_PyObjectWrapper *Get(
       const omu_PyObjectWrapper * def
       );
    //
    // Argument parsing methods
    //
    virtual void Begin();    // begin pulling args, reset cursor (optional)  
    virtual void End(cow_String nestingStr="");
         // end pulling arguments, raise exceptions.
         // the nestingStr argument used to pass a suffix to the error msg when
         //     parsing a tuple inside another. i.e. " of arg 3"
  
    void Optional(); // Get()s following Optional will not raise # arg errors
    bool Error();    // Returns True if there was an error in the previous
                     // set of gets. (used to implement overloading)
    bool Alternative(); // another form of overloading, allows the client to
                        // re-try the previous get.

    // Conversions

    // Convert to cow_Lists. If the types in the prim tuple are not
    // homogeneous these converstion operators will throw type errors.
    cow_ListBool	    AsListBool();
    cow_ListInt		    AsListInt();
    cow_ListFloat	    AsListFloat();
    cow_ListString	    AsListString();
    cow_ListTranslatedString AsListTranslatedString();
    cow_ListDouble       AsListDouble();

    cow_2DArrayDouble	    As2DArrayDouble();
    cow_2DArrayString	    As2DArrayString();

    cow_MapString2Float	    AsMapString2Float();
    
    omu_Primitive &operator[](int i);
    const omu_Primitive &operator[](int i) const;

    void SetNestedErrorStr(const cow_String &str)
         { nestedErrorStr = str; }
    const cow_String & ConstGetNestedErrorStr() const
         { return nestedErrorStr; }
    
    //
    // Messaging/database interface
    //

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

  protected:
    // arguments used when parsing args

    int size;        // max size of tuple
    int cursor;      // current position

    int reqcount;    // number of required arguments
    int argcount;    // total number of required and optional arguments
    bool isOptional; // was optional called?

    // Error condition flags.
    int errorArg;    // There was an error in this argument (1 based)
    cow_String typeFound; 
    cow_String typeExpected; 

    int numGets;     // count on number of gets

    omu_PrimitiveCowList  primVector;

    // Non-persistent data, not included in WriteData/ReadData
    // Provide a context string for error messages in nested tuples
    cow_String  nestedErrorStr;
  private:
    bool CountError(); // Returns true if there is an error in the number
                       // of arguments.
    void SetTypeError(const cow_String &Expected);
    void IncrementCursor();

  public:

    // Messaging/Database interface
    omu_PrimTuple( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

  private:

    cls_Uid m_ClsUid;
};



#endif  // #ifdef omu_PrimTuple_h
