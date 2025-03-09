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
// File Name: omu_PrimSequence.h
// 
// Description:
//    Defines the callable interface on sequence objects.  
//    This interface will support the sequence methods needed in support 
//    of python sequences
//
//    This interface is for importing Python Sequence objects into ABAQUS.
//


#ifndef omu_PrimSequence_h
#define omu_PrimSequence_h

//
// Begin Local Includes
//
#include <omu_Primitive.h>

//
// End Local Includes
//

//
// Forward Declarations
//
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;


class omu_PrimSequence : public omu_Primitive
{
  public:
    virtual ~omu_PrimSequence() {}

    //
    // Primitive interface
    //
    
    virtual cow_String TypeString() const;

    //
    // Sequence Interface
    // a subset of the Python sequence interface
    //

    // Get the length of the sequence
    virtual int               
    Length() const = 0 ;	

    // get a value from the sequence at given index
    virtual const omu_Primitive *  
    Index(int _index) const = 0 ;

    // create new sequence with range of values from original sequence
    virtual omu_PrimSequence *  
    Slice(int _lo=0, int _hi=INT_MAX) const = 0 ;

    // create new sequence which is original sequence repeated n times
    virtual omu_PrimSequence *   
    Repeat(int _num) const = 0 ; 

    // create new sequence which is original sequence "+" new sequence
    virtual omu_PrimSequence *   
    Concat(omu_PrimSequence *_newSeq) const = 0 ;

    const omu_Primitive &operator[](int i) const 
    {    return *Index(i);  }

    // Are these two needed if this is
    // only for objects imported from Python ?

    // remove value from seq at given index
    // virtual int 
    // Remove(const int32& _index ) = 0 ;  

    // remove range of values from seq 
    // virtual int
    // RemoveSlice(const int32&_lo,  const int32&_hi) = 0 ;

    //
    // Messaging/database interface
    //

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

protected:

    omu_PrimSequence(char _type)
	: omu_Primitive(_type) {}

  public:

    // Messaging/Database interface
    omu_PrimSequence( const cls_ReadVisitor & );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

  private:

    cls_Uid m_ClsUid;
};


// Do not instantiate for abstract class 'omu_PrimSequence'


#endif  // #ifdef omu_PrimSequence_h
