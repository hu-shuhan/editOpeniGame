//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odiK_AnalyticSurfaceShortcut_h
#define odiK_AnalyticSurfaceShortcut_h

// Begin local includes

#include <ddr_Shortcut.h>

class odiK_BaseInstance;
class shp_AnalyticSurface;
class odiK_Part;

// Shortcut declarations
ddr_SHORTCUT_DECL(shp_AnalyticSurface, shp_AnalyticSurfaceShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_BaseInstance, shp_AnalyticSurface, odiK_AnalyticSurfaceInPartInstanceShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_Part, shp_AnalyticSurface, odiK_AnalyticSurfaceInPartShortcut);

#endif // odiK_AnalyticSurfaceShortcut_h
