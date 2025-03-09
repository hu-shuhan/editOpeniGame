//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_SequenceFieldValue_h
#define odb_SequenceFieldValue_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldValue.h>
#include <odb_FieldValueList.h>
#include <odb_SequenceFieldLocation.h>
#include <odb_SequenceSubField.h>

#include <cow_List.h>
// End local includes

COW_LIST_DECL(odb_FieldValueList, odb_FieldValueListList);

//
// Forward declarations
//

class odb_SubField;
class odb_Field;
class ddb_FieldContainer;
class ddb_Assembly;
class odb_FieldOutput;

class odb_SequenceFieldValue : public mem_AllocationOperators
{
friend class odb_Field;

public:
    odb_SequenceFieldValue();

    int size() const;
    const odb_FieldValue constGet(uint index) const; 
    const odb_FieldValue operator[](uint index) const 
        { return  constGet(index); } 
    odb_FieldValue get(uint index);

    const odb_FieldValue value(uint index) const; 

    void append(const odb_FieldValue& val);
    void append(odb_SequenceFieldValue& vals);
    void append(const odb_FieldValueList& vals);

    void clearAll();

private:
    bool m_orphaned;
    odb_SequenceSubField m_subFieldCont;
    odb_FieldValueListList m_valListList;
    odb_FieldValueList* m_FieldValueList;
    int m_searched;
    int m_subIndex;
    int m_numSubValue;
    int numSubField;
    int fieldValueSize;

    void AssertSubFieldIsRead ( int subFieldIndex ) const;

public:    // undocumented and unsupported

    odb_SequenceFieldValue& operator=(const odb_SequenceFieldValue & rhs);
    odb_SequenceFieldValue(const odb_SequenceSubField& sfCon);
    void appendSubField(const odb_SequenceInt& fieldKeys,
			const ddb_FieldContainer& fc,
			const ddb_Assembly& rootAssembly);
    void append(const odb_SubField& subField);

    odb_SequenceFieldLocation createLocations() const;
    odb_SequenceString baseElementTypes() const;
    odb_SequenceDataTypes dataTypes() const;
    odb_SequenceInt subFieldTypes() const;

    odb_SequenceFieldValue getSubset(const odb_Enum::odb_ResultPositionEnum p) const;
    odb_SequenceFieldValue getSubset(const odb_Set& ) const;
    odb_SequenceFieldValue getSubset(const odb_SequenceSequenceFloat& locCoord) const;
    odb_SequenceFieldValue getSubset(const odb_SectionPoint& ) const;
    odb_SequenceFieldValue getSubset(const odb_FieldLocation& ) const;
    odb_SequenceFieldValue getSubset(const odb_Element& e) const;
    odb_SequenceFieldValue getSubset(const odb_Node& n) const;
    odb_SequenceFieldValue getSubset(const odb_Instance& i) const;
    odb_SequenceFieldValue getSubset(const odb_String& elementType) const;
    odb_SequenceFieldValue getScalar(odb_Enum::odb_InvariantEnum i,
				     bool mayUseAlternate = false) const;
    odb_SequenceFieldValue getScalar(odb_String componentLabel,
				     bool mayUseAlternate = false) const;

    // used in field.addData(field)
    bool IsOrphan() const;
    const odb_SequenceSubField& SubField() const;
    inline int numSubFields() const { return numSubField; };

    // sorted values
    const odb_FieldValueListList& FieldValueListList() const;
    const odb_FieldValueList& FieldValueList( int subFieldIndex ) const;

    void FixConnectorField(const odb_FieldOutput& deformation, const ddb_FieldContainer& fieldCon);

    //
    void ClearAll() { clearAll(); }
};

#endif // #ifndef odb_SequenceFieldValue_h
