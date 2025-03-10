//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- Mode: C++ -*-
#ifndef annC_Enums_h
#define annC_Enums_h

// Begin local includes

enum annC_RelativePositionEnm
{
    annC_BOTTOM_LEFT,
    annC_BOTTOM_CENTER,
    annC_BOTTOM_RIGHT,
    annC_CENTER_LEFT,
    annC_CENTER,
    annC_CENTER_RIGHT,
    annC_TOP_LEFT,
    annC_TOP_CENTER,
    annC_TOP_RIGHT
};


enum annC_ArrowHeadStyleEnm
{
    annC_ARROW,
    annC_FILLED_ARROW,
    annC_HOLLOW_CIRCLE,
    annC_FILLED_CIRCLE,
    annC_HOLLOW_DIAMOND,
    annC_FILLED_DIAMOND,
    annC_HOLLOW_SQUARE,
    annC_FILLED_SQUARE,
    annC_NO_HEAD
};

enum annC_LineStyleEnm
{
    annC_SOLID,
    annC_DOTTED,
    annC_DASHED,
    annC_DOT_DASH,
    annC_NO_LINE
};

enum annC_LineThicknessEnm
{
    annC_VERY_THIN,
    annC_THIN,
    annC_MEDIUM,
    annC_THICK
};

enum annC_BackgroundStyleEnm
{
    annC_TRANSPARENT,
    annC_MATCH,
    annC_OTHER
};

enum annC_JustificationEnm
{
    annC_JUSTIFY_LEFT,
    annC_JUSTIFY_CENTER,
    annC_JUSTIFY_RIGHT
};

#endif

