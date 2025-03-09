//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef gsl_TypeTable_h
#define gsl_TypeTable_h

// Begin local includes

#include <typ_TypeTables.h>
#include <typ_Architecture.h>
#include <gsl_FacilityTag.h>

#define TYP_TIX_gsl_Point	((TYP_FAC_gsl << 16) | 0x0001)
#define TYP_TIX_gsl_Matrix	((TYP_FAC_gsl << 16) | 0x0002)
#define TYP_TIX_gsl_Vector	((TYP_FAC_gsl << 16) | 0x0003)
#define TYP_TIX_gsl_Plane       ((TYP_FAC_gsl << 16) | 0x0004)
#define TYP_TIX_gsl_Dot         ((TYP_FAC_gsl << 16) | 0x0005)
#define TYP_TIX_gsl_StLine      ((TYP_FAC_gsl << 16) | 0x0006)
#define TYP_TIX_gsl_BBox      	((TYP_FAC_gsl << 16) | 0x0007)
#define TYP_TIX_gsl_Segment     ((TYP_FAC_gsl << 16) | 0x0008)
#define TYP_TIX_gsl_ArrayPoint  ((TYP_FAC_gsl << 16) | 0x0009)
#define TYP_TIX_gsl_LineArray   ((TYP_FAC_gsl << 16) | 0x000A)
#define TYP_TIX_gsl_ModelSegment  ((TYP_FAC_gsl << 16) | 0x000B)
#define TYP_TIX_gsl_ModelSegmentArray  ((TYP_FAC_gsl << 16) | 0x000C)
#define TYP_TIX_gsl_PlaneArray  ((TYP_FAC_gsl << 16) | 0x000D)
#define TYP_TIX_gsl_DotArray    ((TYP_FAC_gsl << 16) | 0x000E)
extern typ_ArchCompEntry gsl_ArchEntry();


#endif
