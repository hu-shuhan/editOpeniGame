//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FieldOutput_h
#define odb_FieldOutput_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Enum.h>
#include <odb_SequenceFieldValue.h>
#include <odb_SequenceFieldBulkData.h>
#include <odb_SequenceFieldLocation.h>
#include <utf_ResEnum.h>
// End local includes

// Forward declarations
template < class A, class B>
class cow_Map;
typedef cow_Map<int,int> cow_MapInt2Int;
class odiK_BaseInstance;
template <class T>
class ddr_Shortcut;
typedef ddr_Shortcut<odiK_BaseInstance> odiK_BaseInstanceShortcut;

class odb_SectionPoint;
class ddb_Field;
class ddb_Subset;
class ddb_FieldStep;
class ddb_FieldContainer;
class ddb_Assembly;
COW_ARRAYCOW_DECL(ddb_Assembly);
class odb_DatumCsys;
COW_COW_DECL(ddb_FieldStep);
class ddb_FieldFrame;
COW_COW_DECL(ddb_FieldFrame);
class ddb_FieldDescription;
class res_FieldDescription;
class ddb_Model;

class odb_Frame;
COW_ARRAYCOW_DECL(odb_Frame);

template <class Type> class res_Holder;
typedef res_Holder<float> res_FieldDataFloat;

class omi_Timer;

class odb_FieldOutput : public mem_AllocationOperators
{
    friend class odb_Frame;
    friend class odb_MathUtil;

public:   // documented and supported
    // member access
    odb_String name() const {return m_name;}
    odb_String description() const {return m_description;}
    odb_Enum::odb_DataTypeEnum type() const {return m_dataType;}
    odb_Enum::odb_DataSubtypeEnum subtype() const {return m_dataSubtype;}
    bool isComplex() const { return m_isComplex; }
    odb_SequenceInvariant validInvariants() const {return m_validInvar;}
    odb_SequenceString componentLabels() const {return m_componentLabels;}
    odb_SequenceString baseElementTypes() const;
    odb_SequenceDataTypes dataTypes() const;
    //
    odb_FieldOutput getSubset(const odb_Enum::odb_ResultPositionEnum p,
                  bool odbReadOnly = false,
                  bool extrapOrientation = true) const;
    odb_FieldOutput getSubset(const odb_Set& region) const;
    odb_FieldOutput getSubset(const odb_SequenceSequenceFloat& locCoord) const;
    odb_FieldOutput getSubset(const odb_SectionPoint& ) const;
    odb_FieldOutput getSubset(const odb_FieldLocation& ) const;
    odb_FieldOutput getSubset(const odb_Element& ) const;
    odb_FieldOutput getSubset(const odb_Node& ) const;
    odb_FieldOutput getSubset(const odb_Instance& ) const;
    odb_FieldOutput getSubset(const odb_String& ) const;
    odb_FieldOutput getScalarField(odb_Enum::odb_InvariantEnum i) const;
    odb_FieldOutput getScalarField(const odb_String& componentLabel) const;
    void setMinClamp(float clamp);
    void setMaxClamp(float clamp);

    void addData( odb_Enum::odb_ResultPositionEnum position,
                  const odb_Instance& instance,
                  const odb_SequenceInt& labels,
                  const odb_SequenceSequenceFloat& data,
                  const odb_SectionPoint& sectionPoint
                      = odb_SectionPoint(),
          const odb_SequenceSequenceFloat& coordinateSystem
              = odb_SequenceSequenceFloat(),
          const odb_SequenceSequenceFloat& conjugateData
              = odb_SequenceSequenceFloat());
    void addData( odb_Enum::odb_ResultPositionEnum position,
                  const odb_Instance& instance,
                  const odb_SequenceInt& labels,
                  const odb_SequenceSequenceFloat& data,
                  const odb_SectionPoint& sectionPoint,
                  const odb_SequenceSequenceSequenceFloat& coordSystem,
          const odb_SequenceSequenceFloat& conjugateData
              = odb_SequenceSequenceFloat());

    void addData( odb_Enum::odb_ResultPositionEnum position,
                  const odb_Set& set,// instance level set
                  const odb_SequenceSequenceFloat& data,
                  const odb_SectionPoint& sectionPoint
                  = odb_SectionPoint(),
                  // const odb_SequenceSequenceSequenceFloat& coordinateSystem
                  //= odb_SequenceSequenceSequenceFloat(),
          const odb_SequenceSequenceFloat& conjugateData
              = odb_SequenceSequenceFloat()
                  );


    void addData(odb_FieldOutput& field);

    // value
    odb_FieldValue values(int i) const;
    const odb_SequenceFieldValue& values() const;
    const odb_SequenceFieldBulkData& bulkDataBlocks() const;

    // locations
    odb_FieldLocation locations(int i) const;
    const odb_SequenceFieldLocation& locations() const;

    // math operators
    odb_FieldOutput operator += ( odb_FieldOutput & );
    odb_FieldOutput operator -= ( odb_FieldOutput & );
    odb_FieldOutput operator *= ( float  );
    odb_FieldOutput operator /= ( float  );

    odb_FieldOutput getTransformedField( const odb_DatumCsys&,
                   odb_Enum::odb_AxisEnum shell2axis=odb_Enum::AXIS_2,
                   double projectionTol=0.52359878 ) const;
    odb_FieldOutput getTransformedField( const odb_DatumCsys&,
                   odb_FieldOutput& deformation,
                   odb_Enum::odb_AxisEnum shell2axis=odb_Enum::AXIS_2,
                   double projectionTol=0.52359878 ) const;
    odb_FieldOutput getTransformedField( const odb_DatumCsys&,
                   odb_FieldOutput& deformation,
                   odb_FieldOutput& rotation,
                   odb_Enum::odb_AxisEnum shell2axis=odb_Enum::AXIS_2,
                   double projectionTol=0.52359878 ) const;
    odb_FieldOutput getConnectorFieldXformedToNodeA(const odb_FieldOutput& deformation= odb_FieldOutput()) const;
    // undocumented and unsupported
    odb_FieldOutput();
    odb_FieldOutput( const odb_String& name,
                     const odb_String& descrip,
                     const odb_Enum::odb_DataTypeEnum datType,
                     const odb_SequenceString& componentLabels,
                 const odb_SequenceInvariant& validInvar,
             const ddb_AssemblyCOW assy,
             const int& frameIndex);
    odb_FieldOutput( const odb_FieldOutput& fieldobj,
                     const ddb_FieldFrameCOW dff,
             const ddb_FieldStepCOW dfs,
             const int& frameIndex);

    odb_FieldOutput getScalarField(odb_Enum::odb_InvariantEnum i,
                   bool mayUseAlternate) const;
    odb_FieldOutput getScalarField(const odb_String& componentLabel,
                   bool mayUseAlternate) const;

    int frameIndex() const {return m_frameIndex;}
    // persistence constructors and members
    bool IsOrphan() const;
    odb_FieldOutput( const odb_String& name,
             const ddb_AssemblyCOW assy,
                     const ddb_FieldFrameCOW dff,
             const ddb_FieldStepCOW dfs,
             const int& frameIndex);
    odb_FieldOutput( const odb_String name,
             const odb_String descrip,
                     const odb_Enum::odb_DataTypeEnum datType,
             const odb_SequenceString componentLabels,
             const odb_SequenceInvariant& validInvar,
             const ddb_AssemblyCOW assy,
                     const ddb_FieldFrameCOW dff,
             const ddb_FieldStepCOW dfs,
             const int& frameIndex,
             int dim=0, int dim2=1,      // for vector & matrix
             const odb_Enum::odb_DataSubtypeEnum subtype=odb_Enum::UNDEFINED_DATASUBTYPE,
             bool isEngineeringTensor=false);

    bool update( const odb_SequenceInt& subNames );

    void addData( odb_Enum::odb_ResultPositionEnum position,
                  const ddb_PartInstance& instance,
                  const odb_SequenceInt& labels,
                  const odb_SequenceSequenceFloat& data,
                  const odb_SectionPoint& sectionPoint,
                  const odb_SequenceSequenceSequenceFloat& coordSystem =
                              odb_SequenceSequenceSequenceFloat(),
          const odb_SequenceSequenceFloat& conjugateData =
                              odb_SequenceSequenceFloat());

    // for adding double precision

    void addData( odb_Enum::odb_ResultPositionEnum position,
                  const odb_Instance& instance,
                  const odb_SequenceInt& labels,
                  const odb_SequenceSequenceDouble& data,
                  const odb_SequenceSequenceSequenceDouble& coordSystem =
                              odb_SequenceSequenceSequenceDouble(),
          const odb_SequenceSequenceDouble& conjugateData =
                              odb_SequenceSequenceDouble());

    void addDataRaw( odb_Enum::odb_ResultPositionEnum position,
                     const odb_Set& set,// instance level set
                     const float *data, int length, 
                     const odb_SectionPoint& sectionPoint = odb_SectionPoint(),
                     const float *conjugateData = NULL );

    // members
    odb_SequenceInt SubFieldNames() const { return m_subFieldNames; }
    bool hasOrientation() const { return m_hasOrientation; }
    int dim() const {return m_dim;}
    int dim2() const {return m_dim2;}

    const odb_SequenceSubField& SubField() const;

    // mutators
    void setDescription(const odb_String& descrip);
    void setName(const odb_String& descrip);
    void setOrphan(bool isOrphan);
    void readLocations();
    void adjustType();
    void setDataType(odb_Enum::odb_DataTypeEnum dataType) { m_dataType = dataType;}
    void setValidInvariants(const odb_SequenceInvariant& invariants) { m_validInvar = invariants;}
    void setComponentLabels(const odb_SequenceString& componentLabels) { m_componentLabels = componentLabels;};

    // utility
    static void CheckFieldInvariants(
                               const odb_Enum::odb_DataTypeEnum& fieldType,
                               const odb_SequenceInvariant& invar );

    ddb_AssemblyCOW Assembly() { return m_rootAssy; }
    ddb_FieldFrameCOW fieldFrame() const {return m_ddbFieldFrame;};
    ddb_FieldStepCOW fieldStep() const {return m_ddbFieldStep;};

private:
    odb_String                      m_name;
    odb_String                      m_description;
    odb_Enum::odb_DataTypeEnum      m_dataType;
    odb_Enum::odb_DataSubtypeEnum   m_dataSubtype;
    bool                            m_isComplex;
    bool                            m_hasOrientation;
    int                             m_dim, m_dim2; // for vector & matrix
    odb_SequenceInvariant           m_validInvar;
    odb_SequenceString              m_componentLabels;
    mutable odb_SequenceFieldBulkData m_bulkDataBlocks;
    odb_SequenceFieldValue          m_fieldValueCont;
    odb_SequenceFieldLocation       m_fieldLocationCont;

    // persistence members
    bool                m_Orphan;
    ddb_AssemblyCOW     m_rootAssy;
    ddb_FieldFrameCOW   m_ddbFieldFrame;
    ddb_FieldStepCOW    m_ddbFieldStep;
    odb_SequenceInt     m_subFieldNames;
    int                 m_frameIndex;
    bool                m_isEngineeringTensor;

    // internal persistence utility functions
    void CheckComponentNames();
	bool OdbHasSameSurface(const ddb_FieldDescription& fieldDIn, ddb_Set& surfInThisOdb);
    ddb_Field GetDDBField(const ddb_FieldContainer& fieldCont, int subFieldKey);
    void SetInternalData(const ddb_Field& ddbField);
    void CheckForCastToFullTensor(const res_FieldDescription& fieldDescript);

    void addSubFields(int fieldKey, res_FieldDescription& resField);
    void GetClassSubData(
            odb_Enum::odb_ResultPositionEnum position,
            const odb_SequenceInt& Labels,
        const cow_MapInt2Int& positionMap,
        const cow_MapInt2Int& classMap,
            const ddb_Subset& subSet,
            const ddb_ElementClass& elClass,
            const odb_SequenceSequenceFloat& data,
            const odb_SequenceSequenceFloat& conjugateData,
            odb_SequenceSequenceFloat& requestData,  // out
            const odb_SequenceSequenceSequenceFloat& coordSystem,
            odb_SequenceSequenceSequenceFloat& requestCoordSystem  // out
    ) const;

    bool IsCompatibleField(odb_FieldOutput& field);
    void AddDdbFieldsFromApiField(odb_FieldOutput& field, const ddb_Assembly& assembly);

    ddb_Field WriteData(ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceFloat& data,
                        const odb_SequenceSequenceSequenceFloat& coordSystem);
    ddb_Field WriteOrientation(const ddb_FieldDescription& fieldD,
                        int numData,
                        const odb_SequenceSequenceSequenceFloat& coordSystem);
    ddb_Field WriteScalarData(const ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceFloat& data);
    ddb_Field WriteVectorData(const ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceFloat& data,
                        const odb_SequenceSequenceSequenceFloat& coordSystem);
    ddb_Field WriteMatrixData(const ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceFloat& data);
    ddb_Field WriteTensorData(const ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceFloat& data,
                        const odb_SequenceSequenceSequenceFloat& coordSystem);

    ddb_Field WriteData(ddb_FieldDescription& fieldD,
                        const odb_SequenceSequenceDouble& data,
                        const odb_SequenceSequenceSequenceDouble& coordSystem);
    ddb_Field WriteOrientation(const ddb_FieldDescription& fieldD,
                               int numData,
                               const odb_SequenceSequenceSequenceDouble& coordSystem);
    ddb_Field WriteScalarData(const ddb_FieldDescription& fieldD,
                              const odb_SequenceSequenceDouble& data);
    ddb_Field WriteVectorData(const ddb_FieldDescription& fieldD,
                              const odb_SequenceSequenceDouble& data,
                              const odb_SequenceSequenceSequenceDouble& coordSystem);

    ddb_Field getPartDispField( const odiK_BaseInstanceShortcut& ,
                                const odb_SequenceSubField& ) const;

    void extrapolateField(odb_Enum::odb_ResultPositionEnum position,
              const odb_SequenceFieldValue& extrapCont,
              bool odbReadOnly = false,
              bool orphaned = false,
              bool extrapOrient = true);

    void extrapolateSingleField( res_OutputPositionEnm resPosition,
                 odb_Enum::odb_ResultPositionEnum position,
                 ddb_Field sourceField,
                 bool odbReadOnly,
                 bool extrapOrient,
                 const ddb_Model& model,
                 odb_SequenceInt& newSubFieldNames,
                 odb_SequenceFieldValue& newFieldValueCont,
                 odb_SequenceFieldLocation& newFieldLocationCont,
                 ddb_FieldContainer& fc,
                 bool timing,
                 omi_Timer& timer1,
                 omi_Timer& timer2);

    void updateFieldDescription(const ddb_Model& model,
                const ddb_FieldDescription&,
                const res_FieldDescription&,
                const ddb_Field&,
                ddb_FieldDescription&,
                odb_Enum::odb_ResultPositionEnum position,
                res_FieldDataFloat& orientData,
                bool odbReadOnly = false,
                bool extrapOrient = true);


};

COW_COW_DECL(odb_FieldOutput);

#endif // odb_FieldOutput_h
