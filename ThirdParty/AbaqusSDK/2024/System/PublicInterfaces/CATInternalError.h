#ifndef CATLOGICALERROR_H
#define CATLOGICALERROR_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U2
 */

#include "CATError.h"
#include "CATErrorDefs.h"

/**
 * Base class for internal errors.
 * <b>Role</b>: CATInternalError is dedicated to any internal error, such as
 * errors in your own algorithms or processes.
 * You can instantiate it, or derive it to create your own internal error class using
 * @href CATDeclareError and @href CATImplementError macros.
 */
class ExportedByJS0ERROR CATInternalError : public CATError {

  public:
   /** @nodoc */
    CATDeclareError(CATInternalError,CATError)

};

#endif

