//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiC_ResEnum_h
#define odiC_ResEnum_h

// Begin local includes
#include <omu_PrimEnum.h>
// End local includes

enum res_OutputPositionEnm {
     res_UNDEFINED_POSITION,
     res_UNIQUE_NODAL,
     res_INTEGRATION_POINT,
     res_ELEMENT_FACE,
     res_ELEMENT_NODAL,
     res_WHOLE_ELEMENT,
     res_ELEMENT_CENTROID,
     res_WHOLE_REGION,
     res_WHOLE_PART_INSTANCE,
     res_WHOLE_MODEL,
     res_GENERAL_PARTICLE,
     res_SURFACE_FACET,
     res_SURFACE_NODAL,
     res_ELEMENT_FACE_NODAL,
     res_ELEMENT_EDGE_NODAL,
     res_SURFACE_EDGE,
     res_ELEMENT_FACE_INTEGRATION_POINT, //no summation at visualization
     res_SURFACE_INTEGRATION_POINT,      //summation at visualization
     // Add new output positions between SURFACE_EDGE and this comment
     res_NUM_OUTPUT_POSITION
};

typedef omu_PrimEnum<res_OutputPositionEnm,1> res_OutputPositionEnmPE;

enum res_DataTypeEnm {
     res_ENUMERATION,
     res_BOOL,
     res_INT,
     res_SCALAR,
     res_VECTOR,
     res_MATRIX,
     res_QUATERNION_2D,
     res_QUATERNION_3D,
     res_TENSOR,
     res_TENSOR_3D_FULL,
     res_TENSOR_3D_PLANAR,
     res_TENSOR_3D_SURFACE,
     res_TENSOR_2D_PLANAR,
     res_TENSOR_2D_SURFACE,
     res_ARRAY_1D,
     res_NUM_DATA_TYPE
};

typedef omu_PrimEnum<res_DataTypeEnm,1> res_DataTypeEnmPE;

enum res_MatrixTypeEnm {
     res_MATRIX_GENERIC,
     res_MATRIX_SYMMETRIC
};

typedef omu_PrimEnum<res_MatrixTypeEnm,1> res_MatrixTypeEnmPE;

enum res_CoordSystemEnm {
     res_COORDSYSTEM_PRINCIPAL,
     res_COORDSYSTEM_GLOBAL,
     res_COORDSYSTEM_LOCAL,
     res_COORDSYSTEM_NOT_APPLICABLE,
     res_COORDSYSTEM_USER_SPECIFIED
};

typedef omu_PrimEnum<res_CoordSystemEnm,1> res_CoordSystemEnmPE;

enum res_ComponentEnm {
     res_COMPONENT_XX,
     res_COMPONENT_YY,
     res_COMPONENT_ZZ,
     res_COMPONENT_XY,
     res_COMPONENT_XZ,
     res_COMPONENT_YZ,
     res_COMPONENT_YX,
     res_COMPONENT_ZX,
     res_COMPONENT_ZY,
     res_COMPONENT_UNDEFINED
};

typedef omu_PrimEnum<res_ComponentEnm,1> res_ComponentEnmPE;

enum res_InvariantEnm {
     res_MAX_PRINCIPAL,
     res_MID_PRINCIPAL,
     res_MIN_PRINCIPAL,
     res_MAX_INPLANE_PRINCIPAL,
     res_MIN_INPLANE_PRINCIPAL,
     res_OUTOFPLANE_PRINCIPAL,
     res_MISES,
     res_TRESCA,
     res_PRESSURE,
     res_INV3,
     res_MAGNITUDE,
     res_MAX_PRINCIPAL_ABS,
     res_MAX_INPLANE_PRINCIPAL_ABS
};

typedef omu_PrimEnum<res_InvariantEnm,1> res_InvariantEnmPE;

enum res_FieldCompletenessEnm {
     res_FIELD_FULL,
     res_FIELD_PARTIAL,
     res_FIELD_EMPTY
};

typedef omu_PrimEnum<res_FieldCompletenessEnm,1> res_FieldCompletenessEnmPE;

enum res_ClampTypeEnm {
     res_NoClamps,
     res_MinClamp,
     res_MaxClamp,
     res_BothClamps
};

typedef omu_PrimEnum<res_ClampTypeEnm,1> res_ClampTypeEnmPE;

enum res_StorageTypeEnm {
     res_FLOAT,
     res_DOUBLE,
     res_INTEGER,
     res_BOOLEAN,
     res_ENUM
};

typedef omu_PrimEnum<res_StorageTypeEnm,1> res_StorageTypeEnmPE;

enum res_LoadBCTypeEnm {
     res_FIXED_DISPLACEMENT,
     res_FIXED_VELOCITY,
     res_FIXED_ACCELERATION,
     res_X_SYMMETRY,
     res_Y_SYMMETRY,
     res_Z_SYMMETRY,
     res_X_ANTISYMMETRY,
     res_Y_ANTISYMMETRY,
     res_Z_ANTISYMMETRY,
     res_ENCASTRE,
     res_PINNED,
     res_NO_DEFORM,
     res_NO_OVAL,
     res_NO_WARP,
     res_FIXED_TEMPERATURE,
     res_FLUID_CAVITY,
     res_PORE_PRESSURE,
     res_SHELL_TO_SOLID,
     res_CONCENTRATED_LOAD,
     res_DISTRIBUTED_LOAD,
     res_UNDEFINED
};

typedef omu_PrimEnum<res_LoadBCTypeEnm,1> res_LoadBCTypeEnmPE;

enum res_LoadDescriptorEnm {
     res_NO_LOADTYPE,
     res_BX,
     res_BY,
     res_BZ,
     res_BXNU,
     res_BYNU,
     res_BZNU,
     res_CENT,
     res_CENTRIF,
     res_CORIO,
     res_GRAV,
     res_HP,
     res_P,
     res_PNU,
     res_ROTA,
     res_FOUNDATION,
     res_BF,
     res_BFNU,
     res_FLUX,
     res_FLUX_USER,
     res_FILM,
     res_FILM_USER,
     res_R,
     res_Q,
     res_QD,
     res_QNU,
     res_PORE,
     res_PORE_USER,
     res_EBF,
     res_ES,
     res_CBF,
     res_CS,
     res_BODY_CONC_FLUX,
     res_BODY_CONC_FLUX_USER,
     res_CONC_FLUX,
     res_CONC_FLUX_USER,
     res_PPEN };

typedef omu_PrimEnum<res_LoadDescriptorEnm,1> res_LoadDescriptorEnmPE;
 

enum res_PairEnm {
     res_PAIR_NONE,
     res_REAL_IMAGINARY,
     res_MAGNITUDE_PHASE
};

typedef omu_PrimEnum<res_PairEnm,1> res_PairEnmPE;    


enum res_ComplexDisplayEnm {
    res_REAL,
    res_IMAGINARY,
    res_COMPLEX_MAGNITUDE,
    res_COMPLEX_PHASE,
    res_COMPLEX_VAL_AT_ANGLE,
	res_COMPLEX_ENVELOPE_MIN,
	res_COMPLEX_ENVELOPE_MAX,
	res_COMPLEX_ENVELOPE_MAX_ABS
};

typedef omu_PrimEnum<res_ComplexDisplayEnm,1> res_ComplexDisplayEnmPE;
    
enum res_SectionResultsEnm
{
  res_USE_BOTTOM,
  res_USE_TOP,
  res_USE_BOTTOM_AND_TOP,
  res_USE_ENVELOPE
};

typedef omu_PrimEnum<res_SectionResultsEnm,1> res_SectionResultsEnmPE;

#endif
