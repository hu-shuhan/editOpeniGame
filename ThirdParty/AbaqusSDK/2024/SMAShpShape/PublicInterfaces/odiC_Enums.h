//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiC_Enums_h
#define odiC_Enums_h

// Begin local includes

//
// SECTION: Enumerations
//

enum calC_PartInstanceTypeEnm {
    calC_USER_DEFINED,
    calC_CYCLIC_SECTOR
};

enum  calC_VectorRangeTypeEnum {
    res_VECTOR_RANGE_NOT_SET = 0,
    res_VECTOR_RANGE_MAG,                 // Compute range for vector mag.
    res_VECTOR_RANGE_EACH_COMP,           // Compute range for each component.
    res_VECTOR_RANGE_MAG_AND_EACH_COMP    // Compute range for mag. and each component.
};

enum calC_VariableRefinementTypeEnm {
    calC_NO_REFINEMENT,
    calC_INVARIANT,
    calC_COMPONENT
};

enum calC_DisplaySurfaceEnm {
    calC_BOTH_SURFACES,
    calC_TOP_SURFACE,
    calC_BOTTOM_SURFACE
};

enum calC_LabelStyleEnm {
    calC_LABEL_ARABIC_UPPER,
    calC_LABEL_ARABIC_LOWER,
    calC_LABEL_ARABIC_NUMBER,
    calC_LABEL_ROMAN_UPPER,
    calC_LABEL_ROMAN_LOWER,
    calC_LABEL_FILLED_CIRCLE,
    calC_LABEL_FILLED_SQUARE,
    calC_LABEL_FILLED_DIAMOND,
    calC_LABEL_FILLED_TRIANGLE,
    calC_LABEL_HOLLOW_CIRCLE,
    calC_LABEL_HOLLOW_SQUARE,
    calC_LABEL_HOLLOW_DIAMOND,
    calC_LABEL_HOLLOW_TRIANGLE,
    calC_LABEL_PLUS,
    calC_LABEL_CROSS
};

enum calC_MarkerSizeEnm {
    calC_SMALL_MARKER,
    calC_MEDIUM_MARKER,
    calC_LARGE_MARKER
};

enum calC_MarkerTypeEnm {
    calC_FILLED_CIRCLE_MARKER,
    calC_FILLED_SQUARE_MARKER,
    calC_FILLED_DIAMOND_MARKER,
    calC_FILLED_TRIANGLE_MARKER,
    calC_HOLLOW_CIRCLE_MARKER,
    calC_HOLLOW_SQUARE_MARKER,
    calC_HOLLOW_DIAMOND_MARKER,
    calC_HOLLOW_TRIANGLE_MARKER,
    calC_PLUS_MARKER,
    calC_CROSS_MARKER
};

enum calC_LineThicknessEnm {
    calC_THIN_LINE,
    calC_MEDIUM_LINE,
    calC_THICK_LINE
};

enum calC_LineStyleEnm {
    calC_SOLID_LINE,
    calC_DASHED_LINE,
    calC_DOTTED_LINE,
    calC_DASH_DOT_LINE
};

enum calC_PresetColorEnm {
    calC_BLACK,
    calC_RED,
    calC_GREEN,
    calC_BLUE,
    calC_CYAN,
    calC_MAGENTA,
    calC_YELLOW,
    calC_WHITE,
    calC_GREY10,
    calC_GREY20,
    calC_GREY30,
    calC_GREY40,
    calC_GREY50,
    calC_GREY60,
    calC_GREY70,
    calC_GREY80,
    calC_GREY90
};

enum calC_ArrowHeadStyleEnm {
    calC_FILLED_ARROWHEAD,
    calC_HOLLOW_ARROWHEAD,
    calC_NO_ARROWHEAD
};

enum calC_ColorMethodEnm {
    calC_UNIFORM_COLOR,
    calC_SPECTRUM_COLOR,
    calC_COMPONENT_COLOR
};

enum calC_SymmetryTypeEnm {
    calC_NONE_SYMMETRY,
    calC_CYCLIC_SYMMETRY,
    calC_HALF_SYMMETRY,
    calC_QUARTER_SYMMETRY
};

enum odiC_TransformTypeEnm {
    odiC_TRANSFORM_ASIS,
    odiC_TRANSFORM_NODALSYS,
    odiC_TRANSFORM_USERSYS,
    odiC_TRANSFORM_ANGULAR,
    odiC_TRANSFORM_ELEMENT
};

#endif /* odiC_Enums_h */
