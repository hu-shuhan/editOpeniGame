#ifndef CXX_SUPPORT_H_
#define CXX_SUPPORT_H_

#if (_MSC_VER  >= 1900 && _MSC_VER  < 2000)
//MSVC++ 14.x (Visual Studio 2015/2017)
#include "Cxx_vc14.h"
#elif _MSC_VER == 1800
//MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
#include "Cxx_vc12.h"
#elif _MSC_VER == 1700
//MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
#include "Cxx_vc11.h"
#elif _MSC_VER == 1600
//MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
#include "Cxx_vc10.h"
#elif _MSC_VER == 1500
//MSVC++ 10.0 _MSC_VER == 1500 (Visual Studio 2008)
#include "Cxx_vc9.h"
#else
#error "unsupported compiler level"
#endif

#endif  // CXX_SUPPORT_H_
