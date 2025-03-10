//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_AuxiliaryData
//
// Description :  Class odb_AuxliaryData represents data that can be 
//                associated with a diagnostic. The data consists of
//                integers, reals, strings, node labels, and element 
//                labels. 
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_AuxiliaryData_h
#define odb_AuxiliaryData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
// End local includes

// Forward declarations
class dia_AuxiliaryData;
COW_ARRAYCOW_DECL(dia_AuxiliaryData);


////////////////////////////////////////////////////////////////////////////////

class odb_AuxiliaryData  : public mem_AllocationOperators
{
public:
   // Variation number that distinguishes the data within the context
   // of a specific type of diagnostic.
   int variation() const;

   // Integer, real, and string data values
   odb_SequenceInt    intValues() const;
   odb_SequenceDouble realValues() const;
   odb_SequenceString stringValues() const;

   // Element and node labels
   odb_SequenceInt    elementLabels() const;
   odb_SequenceString elementInstanceNames() const;
   odb_SequenceInt    nodeLabels() const;
   odb_SequenceString nodeInstanceNames() const;

   // True if the data contains values (int, real, string, node, element)
   bool hasValues() const;

private:
   dia_AuxiliaryDataCOW m_data;

public:
   // Unsupported
   odb_AuxiliaryData();
   odb_AuxiliaryData(const dia_AuxiliaryData& data);
   odb_AuxiliaryData(const odb_AuxiliaryData& data);
   ~odb_AuxiliaryData();
};

#endif
