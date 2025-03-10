#ifndef CATBaseUnknown_WR_h
#define CATBaseUnknown_WR_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATIAV5Level.h"
#include "JS0CORBA.h"
#include "CATBaseUnknown_var.h"
class CATSysWeakRef;

class CATBaseUnknown_WR
{
public:
  ExportedByJS0CORBA CATBaseUnknown_WR();
  ExportedByJS0CORBA CATBaseUnknown_WR(const CATBaseUnknown_WR &);
  ExportedByJS0CORBA ~CATBaseUnknown_WR();

  ExportedByJS0CORBA const CATBaseUnknown_WR &operator=(const CATBaseUnknown_WR &);
  ExportedByJS0CORBA const CATBaseUnknown_WR &operator=(CATBaseUnknown*);

  ExportedByJS0CORBA operator CATBaseUnknown_var() const;
  /**
   * Return an addrefed component
   */
  ExportedByJS0CORBA CATBaseUnknown *GetComponent() const;

  ExportedByJS0CORBA int operator!() const;

  ExportedByJS0CORBA bool operator==(CATBaseUnknown *obj) const;
  ExportedByJS0CORBA bool operator==(const CATBaseUnknown_WR & iObj) const;
  
#if (__cplusplus < 202002L)
  /** @nodoc Starting from C++20, not required anymore */
  bool operator!=(CATBaseUnknown *obj) const { return !operator==(obj); }
  bool operator!=(const CATBaseUnknown_WR & iObj) const { return !operator==(iObj); }
#else  // C++20
  bool operator==(const CATBaseUnknown_var & rhs) const {
      return operator==(static_cast<CATBaseUnknown*>(rhs));
  }
#endif

private:
  CATSysWeakRef *_WeakRef;
};

#endif //CATBaseUnknown_WR_h
