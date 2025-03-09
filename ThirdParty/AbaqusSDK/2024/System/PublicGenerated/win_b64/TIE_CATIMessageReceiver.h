#ifndef __TIE_CATIMessageReceiver
#define __TIE_CATIMessageReceiver

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIMessageReceiver.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIMessageReceiver */
#define declare_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIMessageReceiver##classe : public CATIMessageReceiver \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
      CATDeclareNotCATBaseUnknownTIEMembers \
   public: \
      CATDeclareTIEMethods(CATIMessageReceiver, classe) \
      CATDeclareIUnknownMethodsForNotCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual void HandleMessage(CATICommMsg *iMessage); \
};



#define ENVTIEdeclare_CATIMessageReceiver(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual void HandleMessage(CATICommMsg *iMessage); \


#define ENVTIEdefine_CATIMessageReceiver(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
void  ENVTIEName::HandleMessage(CATICommMsg *iMessage) \
{ \
 (ENVTIECALL(CATIMessageReceiver,ENVTIETypeLetter,ENVTIELetter)HandleMessage(iMessage)); \
} \


/* Name of the TIE class */
#define class_TIE_CATIMessageReceiver(classe)    TIECATIMessageReceiver##classe


/* Common methods inside a TIE */
#define common_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIMessageReceiver, classe) \
 \
 \
CATImplementNotCATBaseUnknownTIEMethods(CATIMessageReceiver, classe) \
CATImplementIUnknownMethodsForNotCATBaseUnknownTIE(CATIMessageReceiver, classe, 0) \
CATImplementCATBaseUnknownMethodsForTIE(CATIMessageReceiver, classe) \
 \
void  TIECATIMessageReceiver##classe::HandleMessage(CATICommMsg *iMessage) \
{ \
   ((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->HandleMessage(iMessage); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
declare_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
common_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIMessageReceiver, classe) \
 \
 \
CATImplementTIEMeta(CATIMessageReceiver, classe, ENUMTypeOfClass::TIE, NULL, TIECATIMessageReceiver##classe::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
declare_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
common_TIE_CATIMessageReceiver(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIMessageReceiver, classe) \
 \
 \
CATImplementTIEMeta(CATIMessageReceiver, classe, ENUMTypeOfClass::TIEchain, NULL, TIECATIMessageReceiver##classe::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIMessageReceiver(classe) TIE_CATIMessageReceiver(classe)
#else
#define BOA_CATIMessageReceiver(classe) CATImplementBOA(CATIMessageReceiver, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIMessageReceiver(classe) Real_TIE_CATIMessageReceiver(classe,TIEV1)
#define TIEchain_Deprecated_CATIMessageReceiver(classe) Real_TIEchain_CATIMessageReceiver(classe,TIEV1) 
#define TIE_CATIMessageReceiver(classe) Real_TIE_CATIMessageReceiver(classe,TIEV2)
#define TIEchain_CATIMessageReceiver(classe) Real_TIEchain_CATIMessageReceiver(classe,TIEV2) 

#endif
