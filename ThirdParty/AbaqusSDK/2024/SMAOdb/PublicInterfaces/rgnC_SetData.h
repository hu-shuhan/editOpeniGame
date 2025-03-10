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
// File Name: rgnC_SetData.h
//
// Description:
//
//    Header file for Set Data ( ELSET and NSET )
//

#ifndef rgnC_SetData_h
#define rgnC_SetData_h

//
// SECTION: System includes
//

//
// Begin Local includes
//
#include <rgnC_SubsetData.h>
#include <rgnC_SubsetDataList.h>
#include <cow_String.h>
#include <cow_ArrayCOW.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>
//
// Forward declarations
//
class  cls_ClassRegistrar;
class  cls_ReadVisitor;
class  cls_WriteVisitor;

////////////////////////////////////////////////////////////////////////////
//
// Define rgnC_SetData class
//

class rgnC_SetData  : public mem_AllocationOperators
{
public:
    rgnC_SetData( const int sIndex, const cow_String &label, const res_SetTypeEnm setType, const bool internal, bool unsorted = false);

    // Default constructor needed for object materialization
    rgnC_SetData();
    
    virtual ~rgnC_SetData();
    
    bool  Ok() const { return constructionOk; }
    
    // Number of Subsets
    int  NumSubsets() const { return numSubset; }
    
#ifdef HKS_DEBUG
    void Print() const;
#endif
    
    // Name of the set
    const cow_String& Label() const { return setLabel; }
    
    // Get SubSet
    const rgnC_SubsetData&  SubSet( const int  subsetIndex ) const;
    const rgnC_SubsetData&  SubSet( const int  meshIndex,
                    const int  classIndex,
                                    res_SetTypeEnm subsetType ) const;

    void AddSubset(const int meshIndex, 
           const int classIndex,
           const cow_ListInt members,
                   res_SetTypeEnm subsetType = res_NUM_SET_TYPE,
                   bool unsorted = false, 
           const cow_ListInt facets = cow_ListInt() );
    // misc 
    int  SetIndex() const { return setIndex; }
    res_SetTypeEnm  SetType() const { return  setType; }
    bool  InternalSet() const { return internalSet; }
    bool  UnsortedSet() const { return unsorted; }

    // Needed for shortcuts to work
    rgnC_SubsetDataList& SubsetList() { return subsetList; }
    const rgnC_SubsetDataList& ConstSubsetList() const { return subsetList; }

    //  same numerical data, but name etc may differ
    bool IsEqual(const rgnC_SetData &rhs) const;

public: // Database interface
    rgnC_SetData(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    // DDB stuff
    virtual rgnC_SetData* Copy() const;

    static rgnC_SetData* Ctor(rgnC_SetData*);
    static void Dtor(rgnC_SetData*);

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

protected:    
    int  setIndex;
    bool   constructionOk;
    cow_String   setLabel;
    int  numSubset;
    res_SetTypeEnmPE  setType;
    bool  internalSet;
    bool  unsorted;
    rgnC_SubsetDataList subsetList;

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(rgnC_SetData, cow_Virtual);

#endif // #ifndef rgnC_SetData_h
