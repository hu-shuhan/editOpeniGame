#ifndef __TIE_IClassFactory
#define __TIE_IClassFactory

#include <string.h>
#include "CATBaseUnknown.h"
#include "CATMetaClass.h"
#include "CATMacForTie.h"
#include "IClassFactory.h"
#include "JS0DSPA.h"
#include "DSYExport.h"


#define Exported DSYExport
#define Imported DSYImport


/* To link an implementation with the interface IClassFactory */
#define declare_TIE_IClassFactory(classe,TIE_Version) \
 \
      CATForwardDeclareTemplateFunctionSpecialization_##TIE_Version(classe) \
 \
class TIEIClassFactory##classe : public IClassFactory \
{ \
   private: \
      CATDeclareCommonTIEMembers2 \
      CATDeclareNotCATBaseUnknownTIEMembers \
   public: \
      CATDeclareTIEMethods(IClassFactory, classe) \
      CATDeclareIUnknownMethodsForNotCATBaseUnknownTIE \
      CATDeclareCATBaseUnknownMethodsForTIE \
      virtual  HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown *pUnkOuter, const IID & riid, void **ppvObject) ; \
      virtual  HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock) ; \
};



#define ENVTIEdeclare_IClassFactory(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
virtual  HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown *pUnkOuter, const IID & riid, void **ppvObject) ; \
virtual  HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock) ; \


#define ENVTIEdefine_IClassFactory(ENVTIEName,ENVTIETypeLetter,ENVTIELetter) \
HRESULT STDMETHODCALLTYPE  ENVTIEName::CreateInstance( IUnknown *pUnkOuter, const IID & riid, void **ppvObject)  \
{ \
return (ENVTIECALL(IClassFactory,ENVTIETypeLetter,ENVTIELetter)CreateInstance(pUnkOuter,riid,ppvObject)); \
} \
HRESULT STDMETHODCALLTYPE  ENVTIEName::LockServer( BOOL fLock)  \
{ \
return (ENVTIECALL(IClassFactory,ENVTIETypeLetter,ENVTIELetter)LockServer(fLock)); \
} \


/* Name of the TIE class */
#define class_TIE_IClassFactory(classe)    TIEIClassFactory##classe


/* Common methods inside a TIE */
#define common_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
/* Static initialization */ \
CATDefineCommonTIEMembers2(IClassFactory, classe) \
 \
 \
CATImplementNotCATBaseUnknownTIEMethods(IClassFactory, classe) \
CATImplementIUnknownMethodsForNotCATBaseUnknownTIE(IClassFactory, classe, 0) \
CATImplementCATBaseUnknownMethodsForTIE(IClassFactory, classe) \
 \
HRESULT STDMETHODCALLTYPE  TIEIClassFactory##classe::CreateInstance( IUnknown *pUnkOuter, const IID & riid, void **ppvObject)  \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->CreateInstance(pUnkOuter,riid,ppvObject)); \
} \
HRESULT STDMETHODCALLTYPE  TIEIClassFactory##classe::LockServer( BOOL fLock)  \
{ \
   return(((classe *)Tie_Method_##TIE_Version(NecessaryData.ForTIE,ptstat,classe))->LockServer(fLock)); \
} \



/* Macro used to link an implementation with an interface */
#define Real_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
declare_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
common_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIECreation(IClassFactory, classe) \
 \
 \
CATImplementTIEMeta(IClassFactory, classe, ENUMTypeOfClass::TIE, NULL, TIEIClassFactory##classe::MetaObject())


/* Macro used to link an implementation with an interface */
/* This TIE is chained on the implementation object */
#define Real_TIEchain_IClassFactory(classe,TIE_Version) \
 \
 \
declare_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
common_TIE_IClassFactory(classe,TIE_Version) \
 \
 \
/* creator function of the interface */ \
/* encapsulate the new */ \
CATImplementTIEchainCreation(IClassFactory, classe) \
 \
 \
CATImplementTIEMeta(IClassFactory, classe, ENUMTypeOfClass::TIEchain, NULL, TIEIClassFactory##classe::MetaObject())

/* Macro to switch between BOA and TIE at build time */ 
#ifdef CATSYS_BOA_IS_TIE
#define BOA_IClassFactory(classe) TIE_IClassFactory(classe)
#else
#define BOA_IClassFactory(classe) CATImplementBOA(IClassFactory, classe)
#endif


/* Macros used to link an implementation with an interface */
#define TIE_Deprecated_IClassFactory(classe) Real_TIE_IClassFactory(classe,TIEV1)
#define TIEchain_Deprecated_IClassFactory(classe) Real_TIEchain_IClassFactory(classe,TIEV1) 
#define TIE_IClassFactory(classe) Real_TIE_IClassFactory(classe,TIEV2)
#define TIEchain_IClassFactory(classe) Real_TIEchain_IClassFactory(classe,TIEV2) 

#endif
