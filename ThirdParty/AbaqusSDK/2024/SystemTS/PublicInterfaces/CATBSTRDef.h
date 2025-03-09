
#ifndef CATBSTRDef_H
#define CATBSTRDef_H

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
 
#ifdef _WINDOWS_SOURCE
# include <windows.h>
#else   // _WINDOWS_SOURCE
/** @nodoc */
typedef wchar_t * BSTR;
#endif  // _WINDOWS_SOURCE

/** 
 * Defines the string type to be used by Automation interfaces.
 */
typedef BSTR CATBSTR;

#endif  // CATBSTRDef_H
