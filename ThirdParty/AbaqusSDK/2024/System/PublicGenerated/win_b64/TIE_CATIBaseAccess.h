#ifndef __TIE_CATIBaseAccess
#define __TIE_CATIBaseAccess

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIBaseAccess.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIBaseAccess */
#define declare_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIBaseAccess##classe : public CATIBaseAccess \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
   public: \
      CATDeclareTIEMethods(CATIBaseAccess, classe) \
      CATDeclareIUnknownMethodsForCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT GiveAccurateExposedInterface(CATBaseDispatch** oBasePointer); \
};



#define ENVTIEdeclare_CATIBaseAccess(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT GiveAccurateExposedInterface(CATBaseDispatch** oBasePointer); \


#define ENVTIEdefine_CATIBaseAccess(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT  ENVTIEName::GiveAccurateExposedInterface(CATBaseDispatch** oBasePointer) \
{ \
return (ENVTIECALL(CATIBaseAccess,ENVTIETypeLetter,ENVTIELetter)GiveAccurateExposedInterface(oBasePointer)); \
} \


/* Name of the TIE class */
#define class_TIE_CATIBaseAccess(classe)    TIECATIBaseAccess##classe


/* Common methods inside a TIE */
#define common_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIBaseAccess, classe) \
 \
 \
CATImplementTIEMethods(CATIBaseAccess, classe) \
CATImplementIUnknownMethodsForCATBaseUnknownTIE(CATIBaseAccess, classe, 1) \
CATImplementCATBaseUnknownMethodsForTIE(CATIBaseAccess, classe) \
 \
HRESULT  TIECATIBaseAccess##classe::GiveAccurateExposedInterface(CATBaseDispatch** oBasePointer) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->GiveAccurateExposedInterface(oBasePointer)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
declare_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
common_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIBaseAccess, classe) \
 \
 \
CATImplementTIEMeta(CATIBaseAccess, classe, ENUMTypeOfClass::TIE, CATIBaseAccess::MetaObject(), CATIBaseAccess::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIBaseAccess(classe,TIE_Version) \
 \
 \
declare_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
common_TIE_CATIBaseAccess(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIBaseAccess, classe) \
 \
 \
CATImplementTIEMeta(CATIBaseAccess, classe, ENUMTypeOfClass::TIEchain, CATIBaseAccess::MetaObject(), CATIBaseAccess::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIBaseAccess(classe) TIE_CATIBaseAccess(classe)
#else
#define BOA_CATIBaseAccess(classe) CATImplementBOA(CATIBaseAccess, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIBaseAccess(classe) Real_TIE_CATIBaseAccess(classe,TIEV1)
#define TIEchain_Deprecated_CATIBaseAccess(classe) Real_TIEchain_CATIBaseAccess(classe,TIEV1) 
#define TIE_CATIBaseAccess(classe) Real_TIE_CATIBaseAccess(classe,TIEV2)
#define TIEchain_CATIBaseAccess(classe) Real_TIEchain_CATIBaseAccess(classe,TIEV2) 

#endif
