#ifndef __TIE_CATIAutomationResolution
#define __TIE_CATIAutomationResolution

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIAutomationResolution.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIAutomationResolution */
#define declare_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIAutomationResolution##classe : public CATIAutomationResolution \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
   public: \
      CATDeclareTIEMethods(CATIAutomationResolution, classe) \
      CATDeclareIUnknownMethodsForCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT GetResolution(CATIScriptMethodCall *& oResolution, CATBaseDispatch * iObjectToResolve) ; \
};



#define ENVTIEdeclare_CATIAutomationResolution(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT GetResolution(CATIScriptMethodCall *& oResolution, CATBaseDispatch * iObjectToResolve) ; \


#define ENVTIEdefine_CATIAutomationResolution(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT  ENVTIEName::GetResolution(CATIScriptMethodCall *& oResolution, CATBaseDispatch * iObjectToResolve)  \
{ \
return (ENVTIECALL(CATIAutomationResolution,ENVTIETypeLetter,ENVTIELetter)GetResolution(oResolution,iObjectToResolve)); \
} \


/* Name of the TIE class */
#define class_TIE_CATIAutomationResolution(classe)    TIECATIAutomationResolution##classe


/* Common methods inside a TIE */
#define common_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIAutomationResolution, classe) \
 \
 \
CATImplementTIEMethods(CATIAutomationResolution, classe) \
CATImplementIUnknownMethodsForCATBaseUnknownTIE(CATIAutomationResolution, classe, 1) \
CATImplementCATBaseUnknownMethodsForTIE(CATIAutomationResolution, classe) \
 \
HRESULT  TIECATIAutomationResolution##classe::GetResolution(CATIScriptMethodCall *& oResolution, CATBaseDispatch * iObjectToResolve)  \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->GetResolution(oResolution,iObjectToResolve)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
declare_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
common_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIAutomationResolution, classe) \
 \
 \
CATImplementTIEMeta(CATIAutomationResolution, classe, ENUMTypeOfClass::TIE, CATIAutomationResolution::MetaObject(), CATIAutomationResolution::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
declare_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
common_TIE_CATIAutomationResolution(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIAutomationResolution, classe) \
 \
 \
CATImplementTIEMeta(CATIAutomationResolution, classe, ENUMTypeOfClass::TIEchain, CATIAutomationResolution::MetaObject(), CATIAutomationResolution::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIAutomationResolution(classe) TIE_CATIAutomationResolution(classe)
#else
#define BOA_CATIAutomationResolution(classe) CATImplementBOA(CATIAutomationResolution, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIAutomationResolution(classe) Real_TIE_CATIAutomationResolution(classe,TIEV1)
#define TIEchain_Deprecated_CATIAutomationResolution(classe) Real_TIEchain_CATIAutomationResolution(classe,TIEV1) 
#define TIE_CATIAutomationResolution(classe) Real_TIE_CATIAutomationResolution(classe,TIEV2)
#define TIEchain_CATIAutomationResolution(classe) Real_TIEchain_CATIAutomationResolution(classe,TIEV2) 

#endif
