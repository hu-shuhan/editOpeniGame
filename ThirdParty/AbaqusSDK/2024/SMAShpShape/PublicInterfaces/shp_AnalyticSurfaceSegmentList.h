//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef shp_AnalyticSurfaceSegmentList_h
#define shp_AnalyticSurfaceSegmentList_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: shp_AnalyticSurfaceSegmentList.h
//
// Description:  Definition of an array of analytical surface segments.
//

// Includes

// Begin local includes
#include <cls_xo1DArrayList.h>
#include <shp_AnalyticSurfaceSegment.h>
// Forward Declarations


///////////////////////////////////////////////////////////////////////////////
//
// Array of connector potentials
//

CLS_xo1DARRAYLIST_DECL(shp_AnalyticSurfaceSegment, shp_AnalyticSurfaceSegmentList);

#endif // shp_AnalyticSurfaceSegmentList_h

