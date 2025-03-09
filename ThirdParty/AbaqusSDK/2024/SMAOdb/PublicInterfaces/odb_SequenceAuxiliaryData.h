//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_SequenceAuxiliaryData
//
// Description :  A sequence of odb_AuxiliaryData objects
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_SequenceAuxiliaryData_h
#define odb_SequenceAuxiliaryData_h


// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <cls_ListObj.h>
// End local includes

// Forward Declarations
class odb_AuxiliaryData;
class dia_AuxiliaryData;
COW_ARRAYCOW_DECL(dia_AuxiliaryData);
CLS_LISTOBJ_DECL(dia_AuxiliaryData, dia_AuxiliaryDataList);


class odb_SequenceAuxiliaryData : public mem_AllocationOperators
{
public:
   // Returns the number of odb_AuxiliaryData objects contained in
   // the sequence
   int size() const;

   // Returns an odb_AuxiliaryData object by zero-based index. If 
   // the index is out of bounds then an exception of type odb_Exception 
   // will be thrown.  
   odb_AuxiliaryData get(int index) const;
   odb_AuxiliaryData operator[](int index) const;

private:
   dia_AuxiliaryDataList m_list;

public: 
   // Unsupported
   odb_SequenceAuxiliaryData();
   odb_SequenceAuxiliaryData(const dia_AuxiliaryDataList& list);
   odb_SequenceAuxiliaryData(const odb_SequenceAuxiliaryData& sequence);
   ~odb_SequenceAuxiliaryData();
};


#endif
