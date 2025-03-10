#ifndef CATSPECERROR_H
#define CATSPECERROR_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

//-----------------------------------------------------------------------------
// Abstract:	Class of errors pertaining to input
//-----------------------------------------------------------------------------
// Usage:	To be instantiated and thrown
//-----------------------------------------------------------------------------
// Inheritance: Derived from CATError
//-----------------------------------------------------------------------------
#include "CATError.h"
#include "CATErrorDefs.h"

/**
 * Base class for input errors.
 * <b>Role</b>: CATInputError is dedicated to any input error, especially
 * end user input errors.
 * You can instantiate it, or derive it to create your own input error class using
 * @href CATDeclareError and @href CATImplementError macros.
 */
class ExportedByJS0ERROR CATInputError : public CATError {

  public:
    /** @nodoc */
    CATDeclareError(CATInputError,CATError)

};

#endif

