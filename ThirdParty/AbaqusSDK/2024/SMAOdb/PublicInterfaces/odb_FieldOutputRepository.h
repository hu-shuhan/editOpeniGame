//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_FieldOutputRepository_h
#define odb_FieldOutputRepository_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldOutput.h>
#include <cow_BtreeString2ListInt.h>

// Forward declarations

class ddb_FieldFrame;
class ddb_FieldStep;
COW_COW_DECL(ddb_FieldFrame); 
COW_COW_DECL(ddb_FieldStep); 

class odb_FieldOutputRepositoryIT;

ODB_CONTAINER_DECL(odb_String, odb_FieldOutput, odb_FieldMap);

class odb_FieldDescription: public mem_AllocationOperators
{
 public : 
    odb_String varLabel;
    odb_String varDescription;
    int sectionPointIndex;
    odb_Enum::odb_DataTypeEnum type;
    odb_Enum::odb_ResultPositionEnum position;
    odb_String instanceName;
    bool hasOrientation;
    bool engineeringTensor;
    bool isComplex;
    odb_SequenceString componentLabels;
    odb_SequenceInvariant invariants;
    odb_String setName;
    odb_String baseElementType;
    odb_FieldDescription():varLabel(), varDescription(),
        sectionPointIndex(-1), type(odb_Enum::UNDEFINED_DATATYPE), position(odb_Enum::UNDEFINED_POSITION), instanceName(), hasOrientation(false), engineeringTensor(false), isComplex(false), componentLabels(), invariants(), setName(), baseElementType()
    {};
};

COW_LIST_DECL(odb_FieldDescription, cow_ListFieldDescription);
ODB_SEQUENCE_DECL(odb_FieldDescription, cow_ListFieldDescription,odb_SequenceFieldDescription);



class odb_FieldOutputRepository : public mem_AllocationOperators
{
    friend class odb_Frame;
    friend class odb_FieldOutputRepositoryIT;
public:
    int size() const;
    const odb_FieldOutput& constGet(const odb_String& name) const;
    odb_FieldOutput& get(const odb_String& name);
    const odb_FieldOutput& operator[](const odb_String& name) const {return constGet(name);};
    odb_FieldOutput& operator[](const odb_String& name) {return get(name);};
    bool isMember(const odb_String& name) const;
    const odb_SequenceString getFieldOutputNames() const;
    odb_SequenceFieldDescription getFieldDescriptions() const;
    void release(const odb_String& name) const;
    void release() const;

public: // Undocumented and unsupported
    ~odb_FieldOutputRepository();
    odb_FieldOutputRepository(const odb_FieldOutputRepository&);
    odb_FieldOutputRepository(const ddb_FieldFrame& frame,
			      const ddb_AssemblyCOW& assembly,
			      const ddb_FieldStep& step,
			      const int frameIndex);
private:
    odb_FieldOutputRepository();
    bool ReadField(const odb_String& name) const;
    const odb_FieldMap& getFieldOutputMap() const;
    bool ReadFields();
    void InsertUnlessMember(const odb_String& , const odb_FieldOutput&);
    void modifyLabel(cow_String& varLabel, const cow_String& desc) const;

    ddb_FieldFrameCOW         m_fieldFrame;
    ddb_AssemblyCOW           m_rootAssy;   
    odb_FieldMap              m_map;
    odb_FieldOutput           m_fieldOutput;
    ddb_FieldStepCOW          m_fieldStep;
    int                       m_frameIndex;
    bool                      allFieldRead;
};

class odb_FieldOutputRepositoryIT : public mem_AllocationOperators
{
public:
    odb_FieldOutputRepositoryIT (const odb_FieldOutputRepository& );

    void first() {m_iter.first();}
    void next() {m_iter.next();}
    bool isDone() {return m_iter.isDone();}
    const odb_String& currentKey() {return m_iter.currentKey();}
    const odb_FieldOutput& currentValue() {return m_iter.currentValue();}

private:
    odb_FieldMapIT m_iter;
};

#endif // #ifndef odb_FieldOutputRepository_h
