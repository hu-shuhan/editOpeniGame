
#ifndef CATTypeCode_h
#define CATTypeCode_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATCORBABoolean.h"

#include "CATCORBAAny.h"

#ifndef ExportedByJS0INF
/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0INF)
# define ExportedByJS0INF   DSYExport
#else // __JS0INF
# define ExportedByJS0INF   DSYImport
#endif  // __JS0INF

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
#endif  // !ExportedByJS0INF

#include "CATIIRBase.h"

#include "CATBaseUnknown.h"
#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0INF IID IID_CATTypeCode;
#else
extern "C" const IID IID_CATTypeCode;
#endif
class ExportedByJS0INF CATTypeCode: public CATBaseUnknown
{
  CATDeclareInterface;

public:
virtual boolean  __stdcall equal(CATTypeCode *tc)=0;
virtual TCKind  __stdcall kind()=0;
virtual RepositoryId  __stdcall id()=0;
virtual CORBA_Identifier  __stdcall name()=0;
virtual unsigned long  __stdcall member_count()=0;
virtual CORBA_Identifier  __stdcall member_name(unsigned long index)=0;
virtual CATTypeCode * __stdcall member_type(unsigned long index)=0;
virtual any  *  __stdcall member_label(unsigned long index)=0;
virtual CATTypeCode * __stdcall discriminator_type()=0;
virtual long  __stdcall default_index()=0;
virtual unsigned long  __stdcall length()=0;
virtual CATTypeCode * __stdcall content_type()=0;
virtual long  __stdcall param_count()=0;
virtual any  *  __stdcall parameter(long index)=0;
};

#endif
