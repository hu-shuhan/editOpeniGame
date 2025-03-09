//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef shp_AnalyticSurfaceSegmentShortcut_h
#define shp_AnalyticSurfaceSegmentShortcut_h

// Begin local includes

#include <ddr_Shortcut.h>
#include <shp_AnalyticSurfaceSegmentList.h>

// Shortcut declarations
ddr_SHORTCUT_DECL(shp_AnalyticSurfaceSegment, shp_AnalyticSurfaceSegmentShortcut);
ddr_SEQUENCE_SHORTCUT_DECL(shp_AnalyticSurfaceSegmentList, shp_AnalyticSurfaceSegment, shp_AnalyticSurfaceSegmentInAnalyticSurfaceSegmentListShortcut);

#endif // shp_AnalyticSurfaceSegmentShortcut_h
