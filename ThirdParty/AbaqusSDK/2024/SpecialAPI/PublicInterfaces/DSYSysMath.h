#ifndef DSYSysMath_H
#define DSYSysMath_H
// COPYRIGHT DASSAULT SYSTEMES 2014
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */


#include <math.h>


// for test of validation of floating-point data
#ifdef _SUNOS_SOURCE
# include <ieeefp.h> // finite SUN
#endif  // _SUNOS_SOURCE

# ifdef _WINDOWS_SOURCE
# include <float.h> // for _finite
# define finite _finite
#endif  // _WINDOWS_SOURCE

#ifdef _IOS_SOURCE
# include <math.h> // for FLT_MAX
# define finite isfinite
#endif  // _IOS_SOURCE

#ifdef _ANDROID_SOURCE
# include <cmath>
#endif  // _ANDROID_SOURCE

#if defined (__HP_aCC) && ( __HP_aCC >= 033000 )
# define finite isfinite
#endif  // (__HP_aCC) && (__HP_aCC >= 033000)

#endif  // DSYSysMath_H
