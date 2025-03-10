//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Enum_h
#define odb_Enum_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <omn_pragma.h>
#include <cow_List.h>
#include <odb_Sequence.h>
#include <omu_PrimEnum.h>
// End local includes

class odb_Enum  : public mem_AllocationOperators
{
public:
    enum odb_SetTypeEnum {
	NODE_SET,
	ELEMENT_SET,
	SURFACE_SET
    };

    enum odb_AnalysisCodeEnum {
	UNKNOWN_ANALYSIS_CODE,
	ABAQUS_STANDARD,
	ABAQUS_EXPLICIT,
	ABAQUS_CFD
    };

    enum odb_PrecisionEnum {
	SINGLE_PRECISION,
	DOUBLE_PRECISION
    };

    enum odb_ElementFaceEnum {
	FACE_UNKNOWN=0,
	END1=1,
	END2,
	END3,
	FACE1=11,
	FACE2,
	FACE3,
	FACE4,
	FACE5,
	FACE6,
	EDGE1=101,
	EDGE2,
	EDGE3,
	EDGE4,
	EDGE5,
	EDGE6,
	EDGE7,
	EDGE8,
	EDGE9,
	EDGE10,
	EDGE11,
	EDGE12,
	EDGE13,
	EDGE14,
	EDGE15,
	EDGE16,
	EDGE17,
	EDGE18,
	EDGE19,
	EDGE20,
	SPOS=1001,
	SNEG=1002,
	SIDE1=1001, // = SPOS
	SIDE2=1002, // = SNEG
	DOUBLE_SIDED=1003 // = DOUBLE SIDED SHELLS
    };


    enum odb_DataTypeEnum {
	BOOLEAN,
	INTEGER,
	SCALAR,
	VECTOR,
	MATRIX,
	TENSOR_3D_FULL,
	TENSOR_3D_PLANAR,
	TENSOR_3D_SURFACE,
	TENSOR_2D_PLANAR,
	TENSOR_2D_SURFACE,
	QUATERNION_3D,
	QUATERNION_2D,
	UNDEFINED_DATATYPE
    };

    enum odb_DataSubtypeEnum {
	UNDEFINED_DATASUBTYPE,
	SYMMETRIC_MATRIX
    };


    enum odb_ResultPositionEnum {
	UNDEFINED_POSITION,
	NODAL,
	ELEMENT_NODAL,
	INTEGRATION_POINT,
	CENTROID,
	ELEMENT_FACE,
	ELEMENT_FACE_INTEGRATION_POINT,
	SURFACE_INTEGRATION_POINT,
	WHOLE_ELEMENT,
	WHOLE_REGION,
	WHOLE_PART_INSTANCE,
	WHOLE_MODEL,
	GENERAL_PARTICLE,
	SURFACE_FACET,
	SURFACE_NODAL,
        NUM_OUTPUT_POSITION
    };

    enum odb_DatumCsysTypeEnum {
      UNKNOWN_CSYSTYPE = 0,
      CARTESIAN,
      CYLINDRICAL,
      SPHERICAL
    };


    enum odb_InvariantEnum {
	UNDEFINED_INVARIANT,
	MAX_PRINCIPAL,
	MID_PRINCIPAL,
	MIN_PRINCIPAL,
	MISES,
	TRESCA,
	PRESS,
	INV3,
	MAGNITUDE,
	MAX_INPLANE_PRINCIPAL,
	MIN_INPLANE_PRINCIPAL,
	OUTOFPLANE_PRINCIPAL,
	MAX_PRINCIPAL_ABS
    };


    enum odb_PartTypeEnum {
	DEFORMABLE_BODY,
	DISCRETE_RIGID_SURFACE,
	ANALYTIC_RIGID_SURFACE
    };


    enum odb_DimensionEnum {
	THREE_D,
	TWO_D_PLANAR,
	AXISYMMETRIC,
	UNKNOWN_DIMENSION
    };


    enum odb_DomainEnum {
	TIME,
	FREQUENCY,
	MODAL,
	ARC_LENGTH
    };


    enum odb_MatchEnum {
	CLOSEST,
	BEFORE,
	AFTER,
	EXACT
    };

    enum odb_MathOperationEnum {
      MULTIPLY,
      DIVIDE,
      SUBTRACT,
      ADD,

      POWER,

      ABS,

      AMP,
      PHASE,
      RE,
      IM,

      ACOS,
      ASIN,
      ATAN,

      COS,
      SIN,
      TAN,

      DEGREE_TO_RADIAN,
      RADIAN_TO_DEGREE,

      EXP,
      EXP10,
      LOG,
      LOG10,
      SQRT,
      
      TRANSPOSE     // matrix transposition
    };


    enum  odb_XYSourceTypeEnm {
      FROM_ODB = 0,
      FROM_ASCII_FILE,
       FROM_KEYBOARD,
      FROM_OPERATION,
      FROM_USER_DEFINED
    };

    enum odb_ProductAddOnEnum {
      AQUA,
      DESIGN,
      BIORID,
      CEL,
      SOLITER,
      CAVPARALLEL,
      ADV2K,
      EEVC,
      IIHS,
      NHTSA,
      WORLDSID,
      HEADFORM_PEEVC,
      AEMDB_SIDE,
      TRL_FWDB_FRONT,
      // Humanetics models
      HMNTCS_Q10,
      HMNTCS_H350,
      HMNTCS_H305,
      HMNTCS_H395,
      HMNTCS_SID2S_SBLC,
      HMNTCS_SID2S_SBLD,
      HMNTCS_ES2,
      HMNTCS_ES2RE,
      HMNTCS_H350_BLST,
      HMNTCS_H305_BLST,
      HMNTCS_H395_BLST,
      HMNTCS_EMHF,
      HMNTCS_FLEXPLI,
      HMNTCS_Q6
    };

    enum odb_AxisEnum {
      AXIS_1,
      AXIS_2,
      AXIS_3
    };
    
    enum odb_StackDirectionEnum {
      STACK_1,
      STACK_2,
      STACK_3,
      STACK_ORIENTATION
    };
    
    enum odb_OrientationMethodEnum
    {
        N1_COSINES,
        CSYS,
        VECT
    };


    enum odb_ElementGeometryClass
    {
        UNKNOWN = 0,
        MASS,
        PLOT,
        TWO_NODE_WELD,
        TWO_NODE_RIGID,
        SPRING,
        BAR2,
        BAR3,
        BEAM2,
        BEAM3,
        TRIA3,
        QUAD4,
        TRIA6,
        QUAD8,
        TETRA4,
        PENTA6,
        HEX8,
        TETRA10,
        PENTA15,
        HEX20
    };

    enum odb_ApiType
    {
	odb_MATERIAL,
	odb_SECTION,
	odb_AMPLITUDE,
        odb_FILTER
    };

    enum odb_RelativePositionEnm
    {
      BOTTOM_LEFT,
      BOTTOM_CENTER,
      BOTTOM_RIGHT,
      CENTER_LEFT,
      CENTER_CENTER,
      CENTER_RIGHT,
      TOP_LEFT,
      TOP_CENTER,
      TOP_RIGHT
    };


    enum odb_ArrowHeadStyleEnm
    {
      ARROW,
      FILLED_ARROW,
      HOLLOW_CIRCLE,
      FILLED_CIRCLE,
      HOLLOW_DIAMOND,
      FILLED_DIAMOND,
      HOLLOW_SQUARE,
      FILLED_SQUARE,
      NO_HEAD
    };

    enum odb_LineStyleEnm
    {
      SOLID,
      DOTTED,
      DASHED,
      DOT_DASH,
      NO_LINE
    };

    enum odb_LineThicknessEnm
    {
      VERY_THIN,
      THIN,
      MEDIUM_THICK,
      THICK
    };

    enum odb_BackgroundStyleEnm
    {
      TRANSPARENT_BACKGROUND,
      MATCH,
      OTHER
    };

    enum odb_LineJustificationEnm
    {
      JUSTIFY_LEFT,
      JUSTIFY_CENTER,
      JUSTIFY_RIGHT
    };

    enum odb_BoxEnm
    {
      OFF,
      ON
    };

    // Use typedefs here as the use of the _DECL macros does not work
    // inside a class and for nested types
    COW_LIST_FWDL(odb_ElementFaceEnum, odb_SeqEF);
    typedef odb_Sequence<odb_ElementFaceEnum,odb_SeqEF> odb_SequenceEF;

    COW_LIST_FWDL(odb_SeqEF, odb_SeqSeqEF);
    typedef odb_Sequence<odb_SequenceEF,odb_SeqSeqEF> odb_SeqSequenceEF;

    COW_LIST_FWDL(odb_InvariantEnum, odb_SeqInv);
    typedef odb_Sequence<odb_InvariantEnum,odb_SeqInv> odb_SequenceInv;

    COW_LIST_FWDL(odb_ProductAddOnEnum, odb_SeqPAO);
    typedef odb_Sequence<odb_ProductAddOnEnum,odb_SeqPAO> odb_SequencePAO;

    COW_LIST_FWDL(odb_DataTypeEnum, odb_SeqDT);
    typedef odb_Sequence<odb_DataTypeEnum, odb_SeqDT> odb_SequenceDT;
};

COW_LIST_BLT_DECL(odb_Enum::odb_ElementFaceEnum, odb_SeqEF);
COW_LIST_DECL(odb_Enum::odb_SeqEF, odb_SeqSeqEF);
COW_LIST_BLT_DECL(odb_Enum::odb_InvariantEnum, odb_SeqInv);
COW_LIST_BLT_DECL(odb_Enum::odb_ProductAddOnEnum, odb_SeqPAO);
COW_LIST_BLT_DECL(odb_Enum::odb_DataTypeEnum, odb_SeqDT);

typedef odb_Enum::odb_SequenceDT odb_SequenceDataTypes;
typedef odb_Enum::odb_SequenceEF odb_SequenceElementFace;
typedef odb_Enum::odb_SeqSequenceEF odb_SequenceSequenceElementFace;
typedef odb_Enum::odb_SequencePAO odb_SequenceProductAddOn;
typedef odb_Enum::odb_ApiType odb_ApiTypeEnm;
typedef odb_Enum::odb_SequenceInv odb_SequenceInvariant;

// Typedefs for symbolic constants
typedef omu_PrimEnum<odb_Enum::odb_AnalysisCodeEnum,1>  odb_AnalysisCodeConst;
typedef omu_PrimEnum<odb_Enum::odb_PrecisionEnum,1>     odb_PrecisionConst;
typedef omu_PrimEnum<odb_Enum::odb_ProductAddOnEnum,1>  odb_ProductAddOnConst;
typedef omu_PrimEnum<odb_Enum::odb_DimensionEnum,1>     odb_DimensionConst;
typedef omu_PrimEnum<odb_Enum::odb_PartTypeEnum,1>      odb_PartTypeConst;
typedef omu_PrimEnum<odb_Enum::odb_DomainEnum,1>        odb_DomainConst;
typedef omu_PrimEnum<odb_Enum::odb_DataTypeEnum,1>      odb_DataTypeConst;
typedef omu_PrimEnum<odb_Enum::odb_DataSubtypeEnum,1>   odb_DataSubtypeConst;
typedef omu_PrimEnum<odb_Enum::odb_ResultPositionEnum,1> odb_ResultPositionConst;
typedef omu_PrimEnum<odb_Enum::odb_InvariantEnum,1>     odb_InvariantConst;
typedef omu_PrimEnum<odb_Enum::odb_ElementFaceEnum,1>   odb_ElementFaceConst;

#endif // odb_Enum_h
