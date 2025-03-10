#ifndef CATRESOURCEERROR_H
#define CATRESOURCEERROR_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

//-----------------------------------------------------------------------------
// Abstract:	Class of errors pertaining to resources
//-----------------------------------------------------------------------------
// Usage:	To be instantiated and thrown
//-----------------------------------------------------------------------------
// Inheritance: Derived from CATError
//-----------------------------------------------------------------------------
#include "CATError.h"
#include "CATErrorDefs.h"

/**
 * Base class for resource errors.
 * <b>Role</b>: CATResourceError is dedicated to any resource error, such as
 * disk failure or memory overflow.
 * You can instantiate it, or derive it to create your own resource error class using
 * @href CATDeclareError and @href CATImplementError macros.
 */
class ExportedByJS0ERROR CATResourceError : public CATError {

  public:
   /** @nodoc */
    CATDeclareError(CATResourceError, CATError)

};

#endif

