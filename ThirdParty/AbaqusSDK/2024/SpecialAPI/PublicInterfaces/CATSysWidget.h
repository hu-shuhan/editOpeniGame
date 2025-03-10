#ifndef CATSysWidget_H
#define CATSysWidget_H

/* COPYRIGHT DASSAULT SYSTEMES 2000 */
/** @CAA2Required */
/************************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS   */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME   */
/************************************************************************/

#include "CATSysObjCClass.h"

/**
 * This header lets manipulate native widget items
 */
#ifdef _WINDOWS_SOURCE
class CWnd;
typedef CWnd * CATSysWidget;

#elif defined (_COCOA_SOURCE) && defined (_DARWIN_SOURCE)
OBJC_CLASS(NSView);
typedef NSView * CATSysWidget;

#elif defined (_COCOA_SOURCE) && defined (_IOS_SOURCE)
OBJC_CLASS(UIView);
typedef UIView * CATSysWidget;

#elif defined (_ANDROID_SOURCE)
#ifdef __cplusplus
class _jobject;
typedef _jobject * jobject;
#else // not __cplusplus
typedef void * jobject;
#endif
typedef jobject CATSysWidget;

#else
struct _WidgetRec;
typedef struct _WidgetRec * Widget;
typedef Widget CATSysWidget;

#endif

#endif  // CATSysWidget_H
