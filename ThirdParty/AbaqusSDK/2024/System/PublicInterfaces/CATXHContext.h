// COPYRIGHT DASSAULT SYSTEMES 2003
#ifndef CATXHCONTEXT_INCLUDE
#define CATXHCONTEXT_INCLUDE
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// Copyright DASSAULT SYSTEMS 1996
//-----------------------------------------------------------------------------
// Abstract:	Context save/restore for exception handling
//-----------------------------------------------------------------------------
// Usage:	Internal class used only by the exception macros
//-----------------------------------------------------------------------------

#include "CATError.h"
#include "CATErrorDefs.h"

// For C++ Native Exceptions
/** @nodoc System FW internal use only */
class ExportedByJS0ERROR CATXHContextCxx {

  public:
    /** @nodoc */
    CATXHContextCxx(int) noexcept;
    /** @nodoc */
    ~CATXHContextCxx() noexcept;
    
    /** @nodoc DO NOT CALL */
    static CATError * CatchBlockProlog(CATXHContextCxx*, int, CATException<CATError> &) noexcept;
    /** @nodoc DO NOT CALL */
    static CATError * CatchBlockProlog(CATXHContextCxx*, int, CATException<CATError> const &) noexcept;
    /** @nodoc DO NOT CALL */
    static CATError * CatchBlockProlog(CATXHContextCxx*, int);
    
    /** @nodoc DO NOT CALL */
    [[noreturn]] static void DoRethrow(CATXHContextCxx*, int, CATError *);
    /** @nodoc DO NOT CALL */
    [[noreturn]] static void DoRethrow(CATXHContextCxx*, int, struct _err_catcatch_info const &);
    
  private:
    void *m_Data;
};

#endif  // CATXHCONTEXT_INCLUDE
