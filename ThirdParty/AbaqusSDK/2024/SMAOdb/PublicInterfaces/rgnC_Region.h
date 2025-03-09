//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef rgnC_Region_h
#define rgnC_Region_h


// Includes

//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <cls_Uid.h>
#include <cow_COW.h>
#include <cow_ListString.h>
#include <cls_IntCOW.h>
#include <cls_xpListString.h>
#include <rgnC_SetDataShortcut.h>
#include <rgnC_SetType.h>
#include <rgnC_SpaceDimBits.h>

//
// Forward declarations
//
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

//
// Region class containing minimum data to be accessed from GUI. The same
// data (space, internal, setTypeBits) exists in rgnF_Set as well.
//
class rgnC_Region  : public mem_AllocationOperators
{
public:

    ///
    // Constructs rgnC_Region object.
    //
    rgnC_Region();

    ///
    // Constructs rgnC_Region object.
    //
    // setName   - in - Name of the set.
    // ownerName - in - Name of the owner.
    // spaceMask - in - Space type of the region.
    // typeMask  - in - Type of region.
    // qInternal - in - Flag to indicate if it is internal region.
    //
    rgnC_Region( const cow_String  & setName,
		 const cow_String  & ownerName = "",
		 rgnC_SpaceBits      spaceMask = rgnC_UNKNOWN_SPACE,
		 rgnC_SetTypeBits    typeMask = rgnC_UNKNOWN_SET_TYPE,
		 bool                qInternal = false );

    ///
    // Constructs rgnC_Region object.
    //
    // setName   - in - Name of the set.
    // ownerName - in - Name of the owner.
    // spaceMask - in - Space type of the region.
    // typeMask  - in - Type of region.
    // qInternal - in - Flag to indicate if it is internal region.
    //
    rgnC_Region( const cow_String      & setName,
		 const cow_ListString  & ownerNames,
		 rgnC_SpaceBits          spaceMask = rgnC_UNKNOWN_SPACE,
		 rgnC_SetTypeBits        typeMask = rgnC_UNKNOWN_SET_TYPE,
		 bool                    qInternal = false );

    ///
    // Constructs rgnC_Region object.
    //
    // setDataShortcut - in - Reference to the data short cut.
    //
    rgnC_Region( const rgnC_SetDataShortcut & setDataShortcut );

    ///
    // Destructs rgnC_Region.
    //
    virtual ~rgnC_Region();

    ///
    // Copies and return a new rgnC_Region object.
    //
    rgnC_Region * Copy() const;

    ///
    // Returns the name of the region.
    //
    const cow_String & Name() const
    {
        return name;

    } // Name


    // Sets the name of the region
    void SetRegionName( const cow_String& rgnName )
    {
	name = rgnName;
	    
    }
    ///
    // Returns the full name of the region.
    //
    // NOTE:
    //    For an instanced set, yields "<instance name>.<set name>"
    //
    cow_String FullName() const; 

    ///
    // Returns the space associated with the region.
    //
    rgnC_SpaceBits Space() const
    {
        return _space;

    } // Space

    ///
    // Returns the type mask.
    //
    rgnC_SetTypeBits TypeBits() const
    {
        return _setTypeBits;

    } // TypeBits

    // Adds skin identifier to the set
    void SetSkinRegion()
    {
	_setTypeBits |= rgnC_SKIN_SET_TYPE;
    }

    // Adds stringer identifier to the set
    void SetStringerRegion()
    {
	_setTypeBits |= rgnC_STRINGER_SET_TYPE;
    }

    ///
    // Checks if the region contains geometry.
    //
    bool ContainsGeometry() const 
    {
        return (_setTypeBits & rgnC_GEOMETRY_SET_TYPE) != 0;

    } // ContainsGeometry

    ///
    // Checks if the region contains nodes.
    //
    bool ContainsNodes() const
    {
        return (_setTypeBits & rgnC_NODE_SET_TYPE) != 0;

    } // ContainsNodes

    ///
    // Checks if the region contains elements.
    //
    bool ContainsElements() const 
    {
        return (_setTypeBits & rgnC_ELEMENT_SET_TYPE) != 0;

    } // ContainsElements

    ///
    // Checks if the region contains only geometry.
    //
    bool ContainsOnlyGeometry() const 
    {
        return  ContainsGeometry() && !ContainsNodes() && !ContainsElements();

    } // ContainsOnlyGeometry

    ///
    // Checks if the region contains only nodes.
    //
    bool ContainsOnlyNodes() const 
    {
        return ContainsNodes() && !ContainsGeometry() && !ContainsElements();

    } // ContainsOnlyNodes

    ///
    // Checks if the region contains only elements.
    //
    bool ContainsOnlyElements() const 
    {
        return ContainsElements() && !ContainsGeometry() && !ContainsNodes();

    } // ContainsOnlyElements

    ///
    // Checks if the region is surface type region.
    //
    bool IsSurfaceRegion() const
    {
        return (_setTypeBits & rgnC_SURF_SET_TYPE) != 0;

    } // IsSurfaceRegion

    ///
    // Checks if the region is skin type region.
    //
    bool IsSkinRegion() const
    {
        return (_setTypeBits & rgnC_SKIN_SET_TYPE) != 0;

    } // IsSkinRegion

    ///
    // Checks if the region is stringer type region.
    //
    bool IsStringerRegion() const
    {
        return (_setTypeBits & rgnC_STRINGER_SET_TYPE) != 0;

    } // IsSurfaceRegion

    ///
    // Checks if the region is set type region.
    //
    bool IsSetRegion() const
    {
        return !IsSurfaceRegion();

    } // IsSetRegion

    ///
    // Checks if the region is valid one.
    //
    // NOTE:
    //    Simplistic validity check (rgnK_Region::IsValid is more reliable)
    //
    bool IsValid() const
    {
        return (_setTypeBits != rgnC_UNKNOWN_SET_TYPE) != 0;

    } // IsValid

    ///
    // Checks if the region is internal.
    //
    bool Internal() const
    {
        return internal;

    } // Internal

    ///
    // Returns the owner names.
    //
    const cow_ListString & OwnerNames() const
    {
        return _ownerNames;

    } // OwnerNames

    ///
    // Returns the owner names.
    //
    cow_ListString & OwnerNames()
    {
        return _ownerNames;

    } // OwnerNames

    ///
    // Returns the reference to the set data short cut.
    //
    const rgnC_SetDataShortcut & SetDataShortcut() const
    {
        return setData;

    } // SetDataShortcut

    ///
    // Clears the set data short cut.
    //
    void ClearSetData()
    {
        setData = rgnC_SetDataShortcut();

    } // ClearSetData

    ///
    // Returns reference to the set data.
    //
    const rgnC_SetData & ConstSet() const;

    ///
    // Compares the set name and the owner names.
    //
    bool CompareNames( const rgnC_Region & region ) const
    {
        return (name == region.name && _ownerNames == region._ownerNames);

    } // CompareNames

public:
    //---------------------------------------------------------------------
    // Operator methods.
    //---------------------------------------------------------------------

    ///
    // Equality operator.
    //
    // region - in - Reference to a region.
    //
    bool operator==( const rgnC_Region & region ) const;

    ///
    // Inequality operator.
    //
    // region - in - Reference to a region.
    //
    bool operator!=(const rgnC_Region&) const;

public:
    //---------------------------------------------------------------------
    // Runtime type defintion methods.
    //---------------------------------------------------------------------

    ///
    // Returns dynamic type identifier.
    //
    virtual typ_typeTag DynTypeId() const;

    ///
    // Returns the type identifier of the class.
    //
    static typ_typeTag TypeId();


public:
    //---------------------------------------------------------------------
    // Database related methods
    //---------------------------------------------------------------------

    ///
    // Constructor used during database read.
    //
    // readVisitor - in - Reference to the read visitor.
    //
    rgnC_Region( const cls_ReadVisitor & readVisitor );

    ///
    // Writes the tracking data using the write visitor.
    //
    // writeVisitor - in - Reference to the write visitor.
    //
    virtual void DBWrite( const cls_WriteVisitor & writeVisitor ) const;

    ///
    // Initializes the meta data for database operations.
    //
    // classRegistrar - Reference to the class registrar.
    //
    static void InitializeMetadata( cls_ClassRegistrar & classRegistrar );

    ///
    // Constructor used during database read for COW.
    //
    // readVisitor - in - Reference to the read visitor.
    //
    static void* Ctor( cls_ReadVisitor & readVisitor );

    ///
    // Destructor used during database read for COW.
    //
    // pIntCOW - in - Pointer to the cow object to be destroyed.
    //
    static void CowDtor( cls_IntCOW * pIntCOW );

private:
    //---------------------------------------------------------------------
    // Persistent Data.
    //---------------------------------------------------------------------

    ///
    // Name of the set
    //
    cow_String name;

    ///
    // Flag to indicate if it is internal set.
    //
    bool internal;

    ///
    // Set data short cut.
    //
    rgnC_SetDataShortcut setData;

    //
    // First name is name of primary object on which it was created. Second
    // name exists if the set was instanced in another primary obejct. It
    // will be name of this other primary object.
    //
    cls_xpListString _ownerNames;

    ///
    // Space mask.
    //
    rgnC_SpaceBits _space;

    ///
    // Type mask.
    //
    rgnC_SetTypeBits _setTypeBits;

private:
    //---------------------------------------------------------------------
    // Data for database operations.
    //---------------------------------------------------------------------

    cls_Uid m_ClsUid;

}; // Class rgnC_Region

COW_COW2_DECL(rgnC_Region, cow_Virtual);

#endif // #ifdef rgnC_Region_h
