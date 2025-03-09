//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_DiagnosticBase
//
// Description :  Generic discussion of types, variations, and auxiliary data
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticBase_h
#define odb_DiagnosticBase_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
// End local includes


// Forward declarations
class odb_SequenceAuxiliaryData;
class dia_MetadataTable;
class dia_DiagnosticBase;
COW_ARRAYCOW2_DECL(dia_DiagnosticBase, cow_Virtual)
class odb_String;


////////////////////////////////////////////////////////////////////////////////

class odb_DiagnosticBase  : public mem_AllocationOperators
{
public:
   // Type id. The type id is a a rather primitive mechanism that we do
   // not expose through the published API. 
   int typeId() const;

   //---------------------------------------------------------------------------
   // Sequence of auxiliary data. Every item in the sequence is 
   // associated with a variation of the type. The length of the 
   // sequence may range from zero to many thousands.
   odb_SequenceAuxiliaryData data() const;

   // Sequence of integers indicating which variation numbers are
   // associated with the auxiliary data. This method is supplied 
   // as a convenience so that clients do not need to derive the
   // variation numbers by iterating over the data.
   odb_SequenceInt variationNumbers() const;

   //---------------------------------------------------------------------------
   // The following methods access information that can be used to
   // interpret the auxiliary data. This "metadata" depends entirely 
   // on the type.

   // Integer indicating the number of variations that are associated
   // with the type. [Note: This is a static value and is NOT the
   // same as the number of variations associated with a specific
   // instance]
   int numberOfVariations() const;

   // String describing the diagnostic. The description applies to
   // all variations.
   odb_String description() const;

   // Sequence of strings describing each of the variations. 
   // [Note: The strings can be empty!] 
   odb_SequenceString variationDescriptions() const;

   // Sequence of strings giving a detailed description of each variation. 
   // The strings may contain format specifiers into which auxiliary data
   // can be substituted.
   odb_SequenceString detailStrings() const;

   // Sequence of strings with auxiliary data substituted.
   odb_SequenceString formattedDetailStrings(int variation) const;

   // String that provides "knowledge" about the diagnostic. Knowledge
   // may include conditions under which the diagnostic is generated,
   // how to get rid of it, etc. The knowledge applies to all variations.
   odb_String knowledgeItem() const;

   // Strings that describe the default format to use when displaying 
   // auxiliary data (i.e. as columns in a table). There is one string
   // for each variation. 
   odb_SequenceString defaultFormats() const;

   // Strings that describe the auxiliary data values. There is one
   // string per variation. If a given variation is associated with
   // more than one value of a given data type (e.g. if variation 2
   // is associated with two reals) then the strings will contain 
   // dot-delimited substrings with one substring per value (i.e. 
   // "Slave surface.MasterSurface")
   odb_SequenceString intDescriptions() const;
   odb_SequenceString realDescriptions() const;
   odb_SequenceString stringDescriptions() const;
   odb_SequenceString nodeDescriptions() const;
   odb_SequenceString elementDescriptions() const;

protected:
   // Derived classes implement this since only they know which
   // table to refer to.
   virtual dia_MetadataTable& getMetadata() const = 0;

private:
   dia_DiagnosticBaseCOW m_base;

public:
   // Unsupported
   odb_DiagnosticBase();
   odb_DiagnosticBase(const dia_DiagnosticBase& base);
   odb_DiagnosticBase(const odb_DiagnosticBase& base);
};


#endif
