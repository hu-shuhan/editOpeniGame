#ifndef CATSysWidgetEx_H
#define CATSysWidgetEx_H

/* COPYRIGHT DASSAULT SYSTEMES 2000 */
/** @CAA2Required */
/************************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS   */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME   */
/************************************************************************/

#include "CATSysObjCClass.h"

// Windows - Predeclaration of CWnd
class CWnd;

// Unix/Linux - Predeclaration of X11 Widget
typedef struct _WidgetRec * Widget;

// Linux - Predeclaration of GtkWidget
typedef struct _GtkWidget GtkWidget;

// MacOS - Predeclaration of NSView
OBJC_CLASS(NSView);

// iOS - Predeclaration of UIView
OBJC_CLASS(UIView);

// Android - Predeclaration of jobject
#ifdef __cplusplus
class _jobject;
typedef _jobject * jobject;
#else // not __cplusplus
typedef void * jobject;
#endif

// Type of data available in CATSysWidgetEx
enum CATSysWidgetExType {
  CATSysWidgetEx_Unspecified = 0,
  CATSysWidgetEx_CWnd,
  CATSysWidgetEx_XWidget,
  CATSysWidgetEx_GtkWidget,
  CATSysWidgetEx_NSView,
  CATSysWidgetEx_UIView,
  CATSysWidgetEx_jobject
};

// Native widget
struct CATSysWidgetEx
{
  CATSysWidgetExType type;
  union {
    CWnd      * cWnd;
    Widget      xWidget;
    GtkWidget * gtkWidget;
    NSView    * nsView;
    UIView    * uiView;
    jobject     jObject;
  };
};

#endif  // CATSysWidgetEx_H
