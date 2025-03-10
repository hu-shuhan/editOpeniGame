//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
//
// File Name: bme_ElementData.h
// 
// Description:
//
//    This file contains the definition of the bme_ElementData class.
//

#ifndef bme_ElementData_h
#define bme_ElementData_h

//
// SECTION: System includes
//

//
// Begin local includes
#include <mem_AllocationOperators.h>
//
#include <BasicConstants.h>

#include <cow_String.h>
#include <cow_IntTableInt.h>
#include <cow_ListInt.h>
#include <omu_VarrayPtr.h>
#include <bme_Types.h>

#include <idb_Utils.h>
#include <bme_ElementClassList.h>

#include <cls_IntCOW.h>
#include <cls_Uid.h>

//
// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class bme_FEMElemList;
class bme_ElementClass;


//
// Class definition
//

class bme_ElementData  : public mem_AllocationOperators
{
public:
    bme_ElementData(bme_ElementClass** c,
		    uint numClasses);	
    bme_ElementData(const bme_ElementData&);

    bme_ElementData();

    virtual ~bme_ElementData();

    // Number of Elements/classes
    uint NumElements() const { return numElem; }
    uint NumClasses() const;

    // Append class
    void AppendClass(bme_ElementClass*); // Client should set FirstElemNum()
    void AppendClasses(const bme_ElementData& narr);

    // Needed for shortcuts
    bme_ElementClassList& GetClassList();
    const bme_ElementClassList& ConstGetClassList() const;
    // HACK: Take out varray
    bme_ElementClassList& GetClasses();
    const bme_ElementClassList& ConstGetClasses() const;



    // called by constructor and ReadData
    void Init();

    virtual bme_ElementData* Copy() const;

    /////////////////////////////////////////////////////////////////////
    // Query functions 
    //
    
    // Get Class
    const bme_ElementClass& GetClass(uint classIndex) const;
    const bme_ElementClass& GetElemClass(uint meshElemIndex) const; 
    const bme_ElementClass& GetFaceClass(const bme_Face&) const;
    const bme_ElementClass& GetEdgeClass(const bme_Edge&) const; 
    bme_ElementClass& GetClassToModify(uint classIndex) ;
	
    // Get Mesh element indices
    int GetMeshElemIndexFromLabel(uint userElemLabel) const;
    int GetMeshElemIndexFromClass(uint classElemIndex,
				    uint classIndex) const; 
    
    // Reverse lookup. Given global element index get the class index and
    // element index within the class
    void GetClassAndElemIndex(uint  meshElemIndex, 
			      uint& classIndex, 
			      uint& classElemIndex ) const;

    // Get element class index.
    uint GetClassIndex(uint meshElemIndex) const;

    // Get element index within the class
    uint GetClassElemIndex(uint meshElemIndex) const;

    // invert the connectivities of the elements. Make the last node
    // first and the first last
    void InvertElementsConnectivities();

    // Class Manipulation
    bool SetElementType(uint elemClassIndex, const cow_String  &newType);

    bool AddElements(uint elemClassIndex, const cow_ListInt &conn);
    bool AddElements(uint elemClassIndex, const cow_ListInt &conn,
                     const cow_ListInt &elemLabels);

    bool AddElementClass(bme_ElementClass* cl);

    bool RemoveElements( const bme_FEMElemList &elemList );

    void ReplaceElemClasses(const bme_ElementClassList&);
    bool RemoveElementClass(uint elemClassIndex);

    // Postpone initialization
    bool RemoveElementClassNoInit(uint elemClassIndex);

    void UpdateConnectivity( const int *node_map );
    
    int GetUserElementLabel(uint meshElemIndex) const;
    bool SetUserElementLabel(uint meshElemIndex, int label);
    bool SetUserElementLabels(const cow_ListInt &newLabels);

    // Get min and max user element label 
    int GetMinUserElementLabel() const;
    int GetMaxUserElementLabel() const;
    int CheckUserElementLabels(const cow_ListInt &labels) const;

    // The next available label is a highwater mark used so new elements are
    // assigned IDs distinct from any previously used IDs.  Only modify
    // if you have a good reason to do so.
    int GetNextAvailableLabel() const;
    void ResetNextLabel(int next=0);   // Zero == current max label + 1

    // Get next available negative element label(for internal elements..ARS*,..)
    int GetNextAvailableInternalLabel() const;


    // Messaging/database interface

    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();



    bme_ElementData *             DeepCopy() const {
        return (bme_ElementData*)DDB_DeepCopyPtr(this, DynTypeId());
    }


public: // Database interface
    bme_ElementData(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void Dtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

    void  GenerateLabel2IdMap();
    void  GenerateMeshElemIndexToClassIndexMap();

protected:

    bme_ElementClassList classes;

    // Inverse mapping for elem-labels.
    bool mapUpdated; // is map up-to-date?
    cow_IntTableInt label2IdMap;
    void UpdateMap() const; 

    // Map from mesh element index to class index
    uint  *meshElemIndexToClassIndexMap;

    uint numElem;
    int _nextAvailableLabel;

private:
    cls_Uid m_ClsUid;

private:
    // Operators -- hidden
    bme_ElementData& operator=(const bme_ElementData&);
    bool operator==(const bme_ElementData&) const;
    bool operator!=(const bme_ElementData&) const;
};

COW_COW2_DECL(bme_ElementData,cow_Virtual);

#endif  // #ifdef bme_ElementData_h
