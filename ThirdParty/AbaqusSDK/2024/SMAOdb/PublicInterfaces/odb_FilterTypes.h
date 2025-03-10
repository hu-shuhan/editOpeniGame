//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_FilterTypes_h
#define odb_FilterTypes_h

// Begin local includes
#include <kflO_Filter.h>
#include <kflO_FilterContainer.h>
#include <kflO_ButterworthFilter.h>
#include <kflO_Chebyshev1Filter.h>
#include <kflO_Chebyshev2Filter.h>
#include <kflO_ApiExtension.h>

typedef kflO_Filter odb_Filter;
typedef kflO_ButterworthFilter odb_ButterworthFilter;
typedef kflO_Chebyshev1Filter odb_Chebyshev1Filter;
typedef kflO_Chebyshev2Filter odb_Chebyshev2Filter;
typedef kflO_FilterContainer odb_FilterContainer;
typedef kflO_FilterContainerIT odb_FilterContainerIT;
typedef kflO_ApiExtension odb_FilterApi;


#endif
