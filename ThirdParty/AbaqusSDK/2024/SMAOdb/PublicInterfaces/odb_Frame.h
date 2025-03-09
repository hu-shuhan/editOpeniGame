//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Frame_h
#define odb_Frame_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_FieldOutputRepository.h>
#include <odb_FieldOutput.h>
#include <odb_Assembly.h>
#include <odb_LoadCase.h>
#include <cow_Map.h>
// End local includes

// Forward declarations
class ddb_Frame;
COW_COW_DECL(ddb_Frame);
class odb_Step;
COW_COW_DECL(odb_Step);

class odb_Frame : public mem_AllocationOperators
{
public:
    odb_Frame(const odb_Frame& frame);
    // member access
    int frameId() const; // Deprecated after 6.6
    int incrementNumber() const;
    int mode() const;
    double frameValue() const;
    double frequency() const;
    odb_String description() const;
    odb_Enum::odb_DomainEnum domain() const;
    odb_Frame associatedFrame();
    int cyclicModeNumber() const;
    const odb_LoadCase& loadCase() const;
    // methods
    bool hasAssociatedFrame() const;
    bool isImaginary() const;

    // fields 
    odb_FieldOutput& FieldOutput (const odb_String& fieldName,
	    const odb_String& fieldDescrib,
	    const odb_Enum::odb_DataTypeEnum& type,
            const odb_SequenceString& componentLabels=odb_SequenceString(),
	    const odb_SequenceInvariant& validInvar=odb_SequenceInvariant(),
	    bool isEngineeringTensor=false);
    odb_FieldOutput& VectorOutput (const odb_String& fieldName,
	                           const odb_String& fieldDescrib,
	                           int width);
    odb_FieldOutput& MatrixOutput (const odb_String& fieldName,
	                           const odb_String& fieldDescrib,
	                           int rows, int cols);
    odb_FieldOutput& SymmetricMatrixOutput (const odb_String& fieldName,
	                                    const odb_String& fieldDescrib,
	                                    int dim);
    odb_FieldOutput& FieldOutput (odb_FieldOutput& field,
                                  const odb_String& name = "",
				  const odb_String& description = "" ); //copy field to frame
    odb_FieldOutputRepository& fieldOutputs ();
    const odb_FieldOutputRepository& fieldOutputs() const;

private:
    int                       m_frameIndex;
    odb_Enum::odb_DomainEnum  m_domain;
    ddb_FrameCOW              m_frame;
    ddb_AssemblyCOW           m_rootAssy;   
    odb_FieldOutputRepository m_fieldContainer;
    odb_StepCOW               m_parent;
    odb_LoadCase              m_loadCase;

public:   // undocumented and unsupported
    odb_Frame();
    ~odb_Frame();
    odb_Frame(int frame_index,
              const ddb_FrameCOW f,
              const ddb_AssemblyCOW a,
              odb_Enum::odb_DomainEnum domain,
	      const odb_Step& parent,
              const odb_LoadCase& lc = odb_LoadCase());

    // 
    bool update();
    const odb_Step& ConstParent() const
        { return m_parent.ConstGet(); }
    odb_Step& Parent();
    const ddb_Frame& ConstDdbFrame() const
        { return m_frame.ConstGet(); }
    ddb_Frame& DdbFrame()
        { return m_frame.Get(); }
    int frameIndex() const;

    // mutator
    void setLoadCase(const odb_LoadCase& lc);
};

COW_ARRAYCOW_DECL(odb_Frame);
COW_MAP_DECL(int,odb_FrameCOW,cow_MapInt2FrameCOW);

#endif // odb_Frame_h
