#ifndef __TIE_CATIStreamMsg
#define __TIE_CATIStreamMsg

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIStreamMsg.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIStreamMsg */
#define declare_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIStreamMsg##classe : public CATIStreamMsg \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
   public: \
      CATDeclareTIEMethods(CATIStreamMsg, classe) \
      CATDeclareIUnknownMethodsForCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT UnstreamData( void *iBuffer, uint32 iLength); \
      virtual HRESULT StreamData( void **oBuffer, uint32 *oLength); \
      virtual HRESULT FreeStreamData( void *Buffer, uint32 iLength); \
      virtual HRESULT SetMessageSpecifications(); \
};



#define ENVTIEdeclare_CATIStreamMsg(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT UnstreamData( void *iBuffer, uint32 iLength); \
virtual HRESULT StreamData( void **oBuffer, uint32 *oLength); \
virtual HRESULT FreeStreamData( void *Buffer, uint32 iLength); \
virtual HRESULT SetMessageSpecifications(); \


#define ENVTIEdefine_CATIStreamMsg(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT  ENVTIEName::UnstreamData( void *iBuffer, uint32 iLength) \
{ \
return (ENVTIECALL(CATIStreamMsg,ENVTIETypeLetter,ENVTIELetter)UnstreamData(iBuffer,iLength)); \
} \
HRESULT  ENVTIEName::StreamData( void **oBuffer, uint32 *oLength) \
{ \
return (ENVTIECALL(CATIStreamMsg,ENVTIETypeLetter,ENVTIELetter)StreamData(oBuffer,oLength)); \
} \
HRESULT  ENVTIEName::FreeStreamData( void *Buffer, uint32 iLength) \
{ \
return (ENVTIECALL(CATIStreamMsg,ENVTIETypeLetter,ENVTIELetter)FreeStreamData(Buffer,iLength)); \
} \
HRESULT  ENVTIEName::SetMessageSpecifications() \
{ \
return (ENVTIECALL(CATIStreamMsg,ENVTIETypeLetter,ENVTIELetter)SetMessageSpecifications()); \
} \


/* Name of the TIE class */
#define class_TIE_CATIStreamMsg(classe)    TIECATIStreamMsg##classe


/* Common methods inside a TIE */
#define common_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIStreamMsg, classe) \
 \
 \
CATImplementTIEMethods(CATIStreamMsg, classe) \
CATImplementIUnknownMethodsForCATBaseUnknownTIE(CATIStreamMsg, classe, 1) \
CATImplementCATBaseUnknownMethodsForTIE(CATIStreamMsg, classe) \
 \
HRESULT  TIECATIStreamMsg##classe::UnstreamData( void *iBuffer, uint32 iLength) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->UnstreamData(iBuffer,iLength)); \
} \
HRESULT  TIECATIStreamMsg##classe::StreamData( void **oBuffer, uint32 *oLength) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->StreamData(oBuffer,oLength)); \
} \
HRESULT  TIECATIStreamMsg##classe::FreeStreamData( void *Buffer, uint32 iLength) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->FreeStreamData(Buffer,iLength)); \
} \
HRESULT  TIECATIStreamMsg##classe::SetMessageSpecifications() \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->SetMessageSpecifications()); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
declare_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
common_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIStreamMsg, classe) \
 \
 \
CATImplementTIEMeta(CATIStreamMsg, classe, ENUMTypeOfClass::TIE, CATIStreamMsg::MetaObject(), CATIStreamMsg::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIStreamMsg(classe,TIE_Version) \
 \
 \
declare_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
common_TIE_CATIStreamMsg(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIStreamMsg, classe) \
 \
 \
CATImplementTIEMeta(CATIStreamMsg, classe, ENUMTypeOfClass::TIEchain, CATIStreamMsg::MetaObject(), CATIStreamMsg::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIStreamMsg(classe) TIE_CATIStreamMsg(classe)
#else
#define BOA_CATIStreamMsg(classe) CATImplementBOA(CATIStreamMsg, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIStreamMsg(classe) Real_TIE_CATIStreamMsg(classe,TIEV1)
#define TIEchain_Deprecated_CATIStreamMsg(classe) Real_TIEchain_CATIStreamMsg(classe,TIEV1) 
#define TIE_CATIStreamMsg(classe) Real_TIE_CATIStreamMsg(classe,TIEV2)
#define TIEchain_CATIStreamMsg(classe) Real_TIEchain_CATIStreamMsg(classe,TIEV2) 

#endif
