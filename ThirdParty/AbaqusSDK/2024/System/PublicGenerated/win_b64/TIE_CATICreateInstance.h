#ifndef __TIE_CATICreateInstance
#define __TIE_CATICreateInstance

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATICreateInstance.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATICreateInstance */
#define declare_TIE_CATICreateInstance(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATICreateInstance##classe : public CATICreateInstance \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
   public: \
      CATDeclareTIEMethods(CATICreateInstance, classe) \
      CATDeclareIUnknownMethodsForCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT __stdcall CreateInstance(void **oPPV) ; \
};



#define ENVTIEdeclare_CATICreateInstance(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT __stdcall CreateInstance(void **oPPV) ; \


#define ENVTIEdefine_CATICreateInstance(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT __stdcall  ENVTIEName::CreateInstance(void **oPPV)  \
{ \
return (ENVTIECALL(CATICreateInstance,ENVTIETypeLetter,ENVTIELetter)CreateInstance(oPPV)); \
} \


/* Name of the TIE class */
#define class_TIE_CATICreateInstance(classe)    TIECATICreateInstance##classe


/* Common methods inside a TIE */
#define common_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATICreateInstance, classe) \
 \
 \
CATImplementTIEMethods(CATICreateInstance, classe) \
CATImplementIUnknownMethodsForCATBaseUnknownTIE(CATICreateInstance, classe, 1) \
CATImplementCATBaseUnknownMethodsForTIE(CATICreateInstance, classe) \
 \
HRESULT __stdcall  TIECATICreateInstance##classe::CreateInstance(void **oPPV)  \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->CreateInstance(oPPV)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
declare_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
common_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATICreateInstance, classe) \
 \
 \
CATImplementTIEMeta(CATICreateInstance, classe, ENUMTypeOfClass::TIE, CATICreateInstance::MetaObject(), CATICreateInstance::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATICreateInstance(classe,TIE_Version) \
 \
 \
declare_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
common_TIE_CATICreateInstance(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATICreateInstance, classe) \
 \
 \
CATImplementTIEMeta(CATICreateInstance, classe, ENUMTypeOfClass::TIEchain, CATICreateInstance::MetaObject(), CATICreateInstance::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATICreateInstance(classe) TIE_CATICreateInstance(classe)
#else
#define BOA_CATICreateInstance(classe) CATImplementBOA(CATICreateInstance, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATICreateInstance(classe) Real_TIE_CATICreateInstance(classe,TIEV1)
#define TIEchain_Deprecated_CATICreateInstance(classe) Real_TIEchain_CATICreateInstance(classe,TIEV1) 
#define TIE_CATICreateInstance(classe) Real_TIE_CATICreateInstance(classe,TIEV2)
#define TIEchain_CATICreateInstance(classe) Real_TIEchain_CATICreateInstance(classe,TIEV2) 

#endif
