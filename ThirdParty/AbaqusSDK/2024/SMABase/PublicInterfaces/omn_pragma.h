//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef OMN_PRAGMA_H
#define OMN_PRAGMA_H
#if defined _WINDOWS_SOURCE || defined _WIN64

// We define default values for the following macros the convenience
// of building AFC (Abaqus For Catia). The values are overridden in
// the config files.

#ifndef WARNING_IGNORE
#define WARNING_IGNORE 4355 4786
#endif

#ifndef WARNING_ERROR
#define WARNING_ERROR 4530 4706
#endif

#pragma warning( disable : WARNING_IGNORE; error : WARNING_ERROR )

#endif // _WINDOWS_SOURCE || _WIN64
#endif // OMN_PRAGMA_H
