#ifndef __TIE_CATIUExitCrypt
#define __TIE_CATIUExitCrypt

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "CATIUExitCrypt.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface CATIUExitCrypt */
#define declare_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIECATIUExitCrypt##classe : public CATIUExitCrypt \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
      CATDeclareNotCATBaseUnknownTIEMembers \
   public: \
      CATDeclareTIEMethods(CATIUExitCrypt, classe) \
      CATDeclareIUnknownMethodsForNotCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual HRESULT Code ( const void *iBuffer, size_t iLen, void **CodedBuffer, size_t *oCodedLen); \
      virtual HRESULT Decode (const void *iCodedBuffer, size_t iCodedLen, void **DecodedBuffer, size_t *oDecodedLen); \
};



#define ENVTIEdeclare_CATIUExitCrypt(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual HRESULT Code ( const void *iBuffer, size_t iLen, void **CodedBuffer, size_t *oCodedLen); \
virtual HRESULT Decode (const void *iCodedBuffer, size_t iCodedLen, void **DecodedBuffer, size_t *oDecodedLen); \


#define ENVTIEdefine_CATIUExitCrypt(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT  ENVTIEName::Code ( const void *iBuffer, size_t iLen, void **CodedBuffer, size_t *oCodedLen) \
{ \
return (ENVTIECALL(CATIUExitCrypt,ENVTIETypeLetter,ENVTIELetter)Code (iBuffer,iLen,CodedBuffer,oCodedLen)); \
} \
HRESULT  ENVTIEName::Decode (const void *iCodedBuffer, size_t iCodedLen, void **DecodedBuffer, size_t *oDecodedLen) \
{ \
return (ENVTIECALL(CATIUExitCrypt,ENVTIETypeLetter,ENVTIELetter)Decode (iCodedBuffer,iCodedLen,DecodedBuffer,oDecodedLen)); \
} \


/* Name of the TIE class */
#define class_TIE_CATIUExitCrypt(classe)    TIECATIUExitCrypt##classe


/* Common methods inside a TIE */
#define common_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(CATIUExitCrypt, classe) \
 \
 \
CATImplementNotCATBaseUnknownTIEMethods(CATIUExitCrypt, classe) \
CATImplementIUnknownMethodsForNotCATBaseUnknownTIE(CATIUExitCrypt, classe, 0) \
CATImplementCATBaseUnknownMethodsForTIE(CATIUExitCrypt, classe) \
 \
HRESULT  TIECATIUExitCrypt##classe::Code ( const void *iBuffer, size_t iLen, void **CodedBuffer, size_t *oCodedLen) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->Code (iBuffer,iLen,CodedBuffer,oCodedLen)); \
} \
HRESULT  TIECATIUExitCrypt##classe::Decode (const void *iCodedBuffer, size_t iCodedLen, void **DecodedBuffer, size_t *oDecodedLen) \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->Decode (iCodedBuffer,iCodedLen,DecodedBuffer,oDecodedLen)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
declare_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
common_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(CATIUExitCrypt, classe) \
 \
 \
CATImplementTIEMeta(CATIUExitCrypt, classe, ENUMTypeOfClass::TIE, NULL, TIECATIUExitCrypt##classe::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
declare_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
common_TIE_CATIUExitCrypt(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(CATIUExitCrypt, classe) \
 \
 \
CATImplementTIEMeta(CATIUExitCrypt, classe, ENUMTypeOfClass::TIEchain, NULL, TIECATIUExitCrypt##classe::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_CATIUExitCrypt(classe) TIE_CATIUExitCrypt(classe)
#else
#define BOA_CATIUExitCrypt(classe) CATImplementBOA(CATIUExitCrypt, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_CATIUExitCrypt(classe) Real_TIE_CATIUExitCrypt(classe,TIEV1)
#define TIEchain_Deprecated_CATIUExitCrypt(classe) Real_TIEchain_CATIUExitCrypt(classe,TIEV1) 
#define TIE_CATIUExitCrypt(classe) Real_TIE_CATIUExitCrypt(classe,TIEV2)
#define TIEchain_CATIUExitCrypt(classe) Real_TIEchain_CATIUExitCrypt(classe,TIEV2) 

#endif
