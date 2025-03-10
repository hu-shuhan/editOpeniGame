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
// File Name: rgnC_SubsetData.h
//
// Description:
//
//    Header file for Set Data ( ELSET and NSET )
//

#ifndef rgnC_SubsetData_h
#define rgnC_SubsetData_h

//
// SECTION: System includes
//

//
// Begin Local includes
#include <omi_limits.h>
#include <omi_IntTableInt.h>
//
#include <BasicConstants.h>
#include <cow_List.h>
#include <cow_ArrayCOW.h>
#include <cow_ListInt.h>

#include <typ_typeTag.h>

#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <cls_xp1DArrayListInt.h>

#include <rgnC_ResEnum.h>

//
// SECTION: Forward declarations
//
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

////////////////////////////////////////////////////////////////////////////
//
// SECTION: Define rgnC_SubsetData class
//

class rgnC_SubsetData  : public mem_AllocationOperators
{
public:
    rgnC_SubsetData( const int  meshIndex,
		     const int  classIndex,
		     const int  subsetIndex,
		     const int  setIndex,		// index of parent set
                     res_SetTypeEnm subsetType,
                     bool unsorted,
		     const cow_ListInt members,
		     const cow_ListInt facets = cow_ListInt());
		       

    // Default constructor needed for object materialization
    rgnC_SubsetData();
    rgnC_SubsetData( const rgnC_SubsetData & );
    
    virtual ~rgnC_SubsetData();

    bool  Ok() const { return constructionOk; }
    bool  UnsortedSubset() const { return unsorted; }

    int  SetIndex() const { return setIndex; }
    int  SubsetIndex() const { return subsetIndex; }
    int  NumItems () const { return idArray.Length(); }
    int  MeshIndex() const { return meshIndex; }
    int  ClassIndex() const { return classIndex; }
    res_SetTypeEnm SubsetType() const { return subsetType; }

    void SetMeshIndex( int mi ) { meshIndex = mi; }
    void SetClassIndex( int ci ) { classIndex = ci; }
    void SetSubsetType( res_SetTypeEnm sst ) { subsetType = sst; }

    cls_xp1DArrayListInt& GetMembers() { safeTable=false; return idArray; }
    cls_xp1DArrayListInt& GetFacets() { return facetIdArray; }

    // DNOTE: The following method provides bounds checking and returns INT_MAX
    int  operator[]( const int n ) const;
    int  FacetId( const int n ) const;

    inline cow_ListInt  Subset()   const { return  idArray; }
    inline cow_ListInt  FacetIds() const { return  facetIdArray; }

    // Given an id, return the index in the subset array.
    // Returns INT_MAX if not found.
    int  LookupIndex( const int  id ) const;

#ifdef HKS_DEBUG
    void Print() const;
#endif

    // QA procedures
    bool operator == (const rgnC_SubsetData &rhs) const;
    bool operator != (const rgnC_SubsetData &rhs) const;

    rgnC_SubsetData & operator = (const rgnC_SubsetData &rhs);

    // DDB stuff
    virtual rgnC_SubsetData* Copy() const;

public: // Database interface
    rgnC_SubsetData(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

protected:
    bool    constructionOk;
    bool    unsorted;
    cls_xp1DArrayListInt idArray;
    cls_xp1DArrayListInt facetIdArray;
    res_SetTypeEnmPE subsetType;
    int   setIndex;
    int   subsetIndex;

    int   meshIndex;
    int   classIndex;

private:
    cls_Uid m_ClsUid;

    bool safeTable;
    mutable omi_IntTableInt idTable;
};


//
// Get item
//

inline int 
rgnC_SubsetData::operator[]( 
    const int  index
) const
{
    if ( idArray.IsEmpty() || index < 0 || index >= idArray.Length() )
	return  INT_MAX;
    else
	return  idArray[index];
}   

inline int 
rgnC_SubsetData::FacetId( 
    const int  index
) const
{
    if ( facetIdArray.IsEmpty() || index < 0 || index >= facetIdArray.Length() )
	return  INT_MAX;
    else
	return  facetIdArray[index];
}   

COW_ARRAYCOW2_DECL(rgnC_SubsetData, cow_Virtual);

#endif // #ifndef rgnC_SubsetData_h
