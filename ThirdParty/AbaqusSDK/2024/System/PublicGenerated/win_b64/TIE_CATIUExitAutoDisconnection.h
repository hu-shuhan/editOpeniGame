#ifndef __TIE_CATIUExitAutoDisconnection
#define __TIE_CATIUExitAutoDisconnection

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIUExitAutoDisconnection.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIUExitAutoDisconnection */
#define declare_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIUExitAutoDisconnection##classe : public CATIUExitAutoDisconnection \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
      CATDeclareNotCATBaseUnknownTIEMembers \
   public: \
      CATDeclareTIEMethods(CATIUExitAutoDisconnection, classe) \
      CATDeclareIUnknownMethodsForNotCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT OnDisconnection (int *oDisconnectionAllowed ); \
};



#define ENVTIEdeclare_CATIUExitAutoDisconnection(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT OnDisconnection (int *oDisconnectionAllowed ); \


#define ENVTIEdefine_CATIUExitAutoDisconnection(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT  ENVTIEName::OnDisconnection (int *oDisconnectionAllowed ) \
{ \
return (ENVTIECALL(CATIUExitAutoDisconnection,ENVTIETypeLetter,ENVTIELetter)OnDisconnection (oDisconnectionAllowed )); \
} \


/* Name of the TIE class */
#define class_TIE_CATIUExitAutoDisconnection(classe)    TIECATIUExitAutoDisconnection##classe


/* Common methods inside a TIE */
#define common_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIUExitAutoDisconnection, classe) \
 \
 \
CATImplementNotCATBaseUnknownTIEMethods(CATIUExitAutoDisconnection, classe) \
CATImplementIUnknownMethodsForNotCATBaseUnknownTIE(CATIUExitAutoDisconnection, classe, 0) \
CATImplementCATBaseUnknownMethodsForTIE(CATIUExitAutoDisconnection, classe) \
 \
HRESULT  TIECATIUExitAutoDisconnection##classe::OnDisconnection (int *oDisconnectionAllowed ) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->OnDisconnection (oDisconnectionAllowed )); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
declare_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
common_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIUExitAutoDisconnection, classe) \
 \
 \
CATImplementTIEMeta(CATIUExitAutoDisconnection, classe, ENUMTypeOfClass::TIE, NULL, TIECATIUExitAutoDisconnection##classe::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
declare_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
common_TIE_CATIUExitAutoDisconnection(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIUExitAutoDisconnection, classe) \
 \
 \
CATImplementTIEMeta(CATIUExitAutoDisconnection, classe, ENUMTypeOfClass::TIEchain, NULL, TIECATIUExitAutoDisconnection##classe::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIUExitAutoDisconnection(classe) TIE_CATIUExitAutoDisconnection(classe)
#else
#define BOA_CATIUExitAutoDisconnection(classe) CATImplementBOA(CATIUExitAutoDisconnection, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIUExitAutoDisconnection(classe) Real_TIE_CATIUExitAutoDisconnection(classe,TIEV1)
#define TIEchain_Deprecated_CATIUExitAutoDisconnection(classe) Real_TIEchain_CATIUExitAutoDisconnection(classe,TIEV1) 
#define TIE_CATIUExitAutoDisconnection(classe) Real_TIE_CATIUExitAutoDisconnection(classe,TIEV2)
#define TIEchain_CATIUExitAutoDisconnection(classe) Real_TIEchain_CATIUExitAutoDisconnection(classe,TIEV2) 

#endif
