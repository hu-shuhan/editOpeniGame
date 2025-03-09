//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef BasicConstants_h
#define BasicConstants_h

// File Name: BasicConstants.h
// 
// Description:
// 
// This header file defines constant values from the MIN and MAX
// macros of the explicitly-sized scalar types. The macros should be
// used in file-scope expressions, in order to avoid generating a
// static initializer. In all other cases, these constants should be
// used to improve type-safety and debug-convenience.

#include <float.h>

// Various mathematical constants are defined as macros, M_*, in math.h
// if XOPEN is supported. Since these are not defined by Visual C++, we
// define our own as HKS_*. As stated above, use the macros only in
// file-scope expressions; use the constant values, double* or float*
// in all other cases.

#define HKS_PI                    3.14159265358979323846
#define HKS_PI_2                  1.57079632679489661923
#define HKS_PI_4                  0.78539816339744830962

// Begin local includes

static const float floatpi   = static_cast<float>(HKS_PI);
static const float floatpi_2 = static_cast<float>(HKS_PI_2);
static const float floatpi_4 = static_cast<float>(HKS_PI_4);

static const double double_tpi   = HKS_PI;
static const double double_tpi_2 = HKS_PI_2;
static const double double_tpi_4 = HKS_PI_4;

#endif     // BasicConstants_h
