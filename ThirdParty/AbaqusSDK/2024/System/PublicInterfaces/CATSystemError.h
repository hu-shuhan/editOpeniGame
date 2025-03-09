#ifndef CATSYSTEMERROR_H
#define CATSYSTEMERROR_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//-----------------------------------------------------------------------------
// Abstract:	The class of errors coming from the system (excluding the ones
//		pertaining to resources)
//-----------------------------------------------------------------------------
// Usage:	Used in CATCatch to filter errors
//-----------------------------------------------------------------------------
#include "CATError.h"
#include "CATErrorDefs.h"

/**
 * Base class for system errors.
 * <b>Role</b>: CATSystemError is dedicated to any system error.
 * You cannot instantiate it. You can just retrieve a pointer to such an error
 * using the error manager static @href CATError#CATGetLastError method.
 */
class ExportedByJS0ERROR CATSystemError : public CATError {

  public:
  /** @nodoc */
    CATDeclareError(CATSystemError,CATError)

};

#endif

