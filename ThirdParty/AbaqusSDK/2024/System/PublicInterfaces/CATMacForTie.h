#include "CATMacForTie_required.h"
#ifndef CATMacForTie_h
#define CATMacForTie_h  42600
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/* COPYRIGHT DASSAULT SYSTEMES 2000 */

#include "CATMacForIUnknown.h"
#include "JS0CORBA.h"
#include "JS0DSPA.h"

/** @nodoc */
class Tie_StackCtx
{
  friend CATBaseUnknown * Tie_StackCtx_FindImpl(CATBaseUnknown const *PtOnCodeExt) noexcept;
private:
  CATBaseUnknown *m_PtOnCodeExt;
  CATBaseUnknown *m_Data;
  Tie_StackCtx const *m_prev;
  
public:
  Tie_StackCtx() noexcept {}
  Exported ~Tie_StackCtx() noexcept;  // Not virtual by intent
  Exported CATBaseUnknown* Run(CATBaseUnknown *PtOnCodeExt, CATBaseUnknown *Data) noexcept;
};

/**
 * @nodoc
 */
#define Tie_Method(Data,ptstat) ( (ptstat) ? (Tie_StackCtx().Run(ptstat,Data)) : (Data) )
#define Tie_Method_TIEV1(Data,ptstat,classe) Tie_Method(Data,ptstat)
#define Tie_Method_TIEV2(Data,ptstat,classe) ( (TIE_IsCodeExtension<classe>()==1) ? (Tie_StackCtx{}.Run(ptstat,Data)) : (Data) )

/**
 * @nodoc
 */
#define CATForwardDeclareTemplateFunctionSpecialization_TIEV1(classe)
#define CATForwardDeclareTemplateFunctionSpecialization_TIEV2(classe) namespace{template <> inline short TIE_IsCodeExtension<classe>();}

#ifdef ENVTIECALL
#undef ENVTIECALL
#endif

#define ENVTIECALL(ENVTIEInterface,ENVTIETypeLetter,ENVTIELetter)  ((ENVTIETypeLetter*)&ENVTIELetter)->

/** @nodoc */
ExportedByJS0CORBA ULONG Tie_AddRef(CATBaseUnknown *, TypeOfClass, LONG *); 
/** @nodoc */
ExportedByJS0DSPA HRESULT Tie_GetTypeInfoCount(IDispatch *, CATMetaClass *, unsigned int *); 
/** @nodoc */
ExportedByJS0DSPA HRESULT Tie_GetTypeInfo(IDispatch *, CATMetaClass *, unsigned int, ULONG, ITypeInfo **); 
/** @nodoc */
ExportedByJS0DSPA HRESULT Tie_GetIDsOfNames(IDispatch *, CATMetaClass *, const IID &, CATWCHAR_T **, unsigned int, ULONG, LONG *); 
/** @nodoc */
ExportedByJS0DSPA HRESULT Tie_Invoke(IDispatch *, CATMetaClass *, LONG, const IID &, ULONG, unsigned short, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *); 

namespace dsy
{
    namespace internal  // Internal use only
    {
        ExportedByJS0CORBA CATMetaClass * Tie_InitMetaObject(IID const &, char const *, CATMetaClass *, CATMetaClass *, TypeOfClass);
    }   // namespace internal
}   // namespace dsy

/**
 * @nodoc
 * Declaration of the common TIE members.
 */
#define CATDeclareCommonTIEMembers \
	static CATBaseUnknown *ptstat; /*For CodeExtension*/

/**
 * @nodoc
 * Initialization of the common static TIE members.
 */
#define CATDefineCommonTIEMembers(interface, classe) \
CATBaseUnknown *TIE##interface##classe::ptstat = NULL;

/**
 * @nodoc 
 */
#define CATImplementTIEMeta(interface, classe, iTypeOfClass, ItfMetaClassPtr, ItfMetaClassPtr2)         \
    /* Define the static member function MetaObject */                                                  \
    CATMacMetaObjectMethodProlog(TIE##interface##classe)                                                \
        meta_object = dsy::internal::Tie_InitMetaObject(IID_##interface, #interface, ItfMetaClassPtr, classe::MetaObject(), iTypeOfClass);\
    CATMacMetaObjectMethodEpilog(TIE##interface##classe)                                                \
                                                                                                        \
    /* To put information into the dictionary */                                                        \
    CATOMInitializerProlog(Dic##interface##classe)                                                      \
        CATFillDictionary Dic##interface##classe(classe::MetaObject(),ItfMetaClassPtr2,CreateTIE##interface##classe);\
    CATOMInitializerEpilog(Dic##interface##classe)

/**
 * @nodoc
 * Declaration of the specific members for TIE which do not derive from CATBaseUnknown.
 */
#define CATDeclareNotCATBaseUnknownTIEMembers \
	TypeOfData NecessaryData; \
	LONG m_cRef; \
	CATBaseUnknown *delegate;

/**
 * @nodoc
 * Declaration of the methods specific to TIE.
 */
#define CATDeclareTIEMethods(interface, classe) \
	TIE##interface##classe(CATBaseUnknown *pt = NULL, CATBaseUnknown *delegue = NULL); \
	virtual ~TIE##interface##classe();

/**
 * @nodoc
 * Implementation of the constructor of a TIE deriving from CATBaseUnknown.
 */
#define CATImplementTIEConstructor(interface, classe) \
TIE##interface##classe::TIE##interface##classe(CATBaseUnknown *pt,CATBaseUnknown *delegue) { \
	/* Thread-safety: the static 'ptstat' member is only used for 'CodeExtension' classes */ \
    Tie_Construct(*this, MetaObject(), pt, classe::ClassId(), classe::MetaObject()->GetTypeOfClass(), ptstat, classe::CreateItself, delegue); \
}

/**
 * @nodoc
 * Implementation of the constructor of a TIE NOT deriving from CATBaseUnknown.
 */
#define CATImplementNotCATBaseUnknownTIEConstructor(interface, classe) \
TIE##interface##classe::TIE##interface##classe(CATBaseUnknown *pt,CATBaseUnknown *delegue) { \
	NecessaryData.ForTIE = NULL; \
	m_cRef = 1; \
	delegate = NULL; \
    /* Thread-safety: the static 'ptstat' member is only used for 'CodeExtension' classes */ \
	Tie_Construct(this, MetaObject(), &NecessaryData.ForTIE, 0, pt, classe::ClassId(), classe::MetaObject()->GetTypeOfClass(), ptstat, classe::CreateItself, delegue, &delegate); \
}


/**
 * @nodoc
 * Implementation of the methods specific to TIE deriving from CATBaseUnknown.
 */
#define CATImplementTIEMethods(interface, classe) \
CATImplementTIEConstructor(interface, classe) \
TIE##interface##classe::~TIE##interface##classe() { \
  Tie_Destruct(*this, MetaObject()->GetTypeOfClass()); \
}

/**
 * @nodoc
 * Implementation of the methods specific to TIE NOT deriving from CATBaseUnknown.
 */
#define CATImplementNotCATBaseUnknownTIEMethods(interface, classe) \
CATImplementNotCATBaseUnknownTIEConstructor(interface, classe) \
TIE##interface##classe::~TIE##interface##classe() { \
  Tie_Destruct(this, &NecessaryData.ForTIE, MetaObject()->GetTypeOfClass(), m_cRef); \
}

/**
 * @nodoc
 * Declaration of the IUnknown methods for TIE.
 */
#define CATDeclareIUnknownMethodsForTIE \
	/*virtual*/ HRESULT __stdcall QueryInterface(const IID &iid, void **ppv) override; \
	/*virtual*/ ULONG __stdcall AddRef() override; \
	/*virtual*/ ULONG __stdcall Release() override;

/**
 * @nodoc
 * Declaration of the IUnknown methods for TIE deriving from CATBaseUnknown.
 */
#define CATDeclareIUnknownMethodsForCATBaseUnknownTIE CATDeclareIUnknownMethodsForTIE

/**
 * @nodoc
 * Declaration of the IUnknown methods for TIE NOT deriving from CATBaseUnknown.
 */
#define CATDeclareIUnknownMethodsForNotCATBaseUnknownTIE CATDeclareIUnknownMethodsForTIE

/**
 * @nodoc
 * Implementation of the IUnknown methods for TIE.
 */
#define CATImplementIUnknownMethodsForTIE(interface, classe, isIDispatch) \
HRESULT __stdcall TIE##interface##classe::QueryInterface(const IID &iid, void **ppv) { \
	return Tie_Query(this, NecessaryData.ForTIE, delegate, MetaObject(), isIDispatch, iid, ppv); \
} \
ULONG __stdcall TIE##interface##classe::AddRef() { \
	return Tie_AddRef(NecessaryData.ForTIE, MetaObject()->GetTypeOfClass(), &m_cRef); \
} \
ULONG __stdcall TIE##interface##classe::Release() { \
	int Destruct = 0; \
	ULONG Compt = Tie_Release(&Destruct, &NecessaryData.ForTIE, delegate, MetaObject()->GetTypeOfClass(), &m_cRef); \
	if (Destruct) delete this; \
	return Compt; \
}

/**
 * @nodoc
 * Implementation of the IUnknown methods for TIE deriving from CATBaseUnknown.
 */
#define CATImplementIUnknownMethodsForCATBaseUnknownTIE(interface, classe, isIDispatch) \
    HRESULT __stdcall TIE##interface##classe::QueryInterface(const IID &iid, void **ppv) { \
        return Tie_Query(*this, MetaObject(), isIDispatch, iid, ppv); \
    } \
    ULONG __stdcall TIE##interface##classe::AddRef() { \
        return Tie_AddRef(NecessaryData.ForTIE, MetaObject()->GetTypeOfClass(), &m_cRef); \
    } \
    ULONG __stdcall TIE##interface##classe::Release() { \
        int Destruct = 0; \
        ULONG Compt = Tie_Release(&Destruct, *this, MetaObject()->GetTypeOfClass()); \
        if (Destruct) delete this; \
        return Compt; \
    }

/**
 * @nodoc
 * Implementation of the IUnknown methods for TIE NOT deriving from CATBaseUnknown.
 */
#define CATImplementIUnknownMethodsForNotCATBaseUnknownTIE(interface, classe, isIDispatch) \
	CATImplementIUnknownMethodsForTIE(interface, classe, isIDispatch)

/**
 * @nodoc
 * Declaration of the IDispatch methods for TIE.
 */
#define CATDeclareIDispatchMethodsForTIE \
	/*virtual*/ HRESULT __stdcall GetTypeInfoCount(unsigned int *oNbOfTypeInfo) override; \
	/*virtual*/ HRESULT __stdcall GetTypeInfo(unsigned int iIndex, ULONG iLangId, ITypeInfo **oPtTypeInfo) override; \
	/*virtual*/ HRESULT __stdcall GetIDsOfNames(const IID &forFutur, CATWCHAR_T **iArrayOfNames, unsigned int iNbNames, ULONG iLangId, LONG *oArrayOflong) override; \
	/*virtual*/ HRESULT __stdcall Invoke(LONG ilongMember, const IID &forFutur,ULONG iLangId, unsigned short iFlags, DISPPARAMS *iPdisparams, VARIANT *oPvaresult, EXCEPINFO *oPexcepinfo, unsigned int *oPuArgErr) override;

/**
 * @nodoc
 * Declaration of the IDispatch methods for TIE deriving from CATBaseUnknown.
 */
#define CATDeclareIDispatchMethodsForCATBaseUnknownTIE CATDeclareIDispatchMethodsForTIE

/**
 * @nodoc
 * Declaration of the IDispatch methods for TIE NOT deriving from CATBaseUnknown.
 */
#define CATDeclareIDispatchMethodsForNotCATBaseUnknownTIE CATDeclareIDispatchMethodsForTIE

/**
 * @nodoc
 * Implementation of the IDispatch methods for TIE.
 */
#define CATImplementIDispatchMethodsForTIE(interface, classe) \
HRESULT __stdcall TIE##interface##classe::GetTypeInfoCount(unsigned int *oNbOfTypeInfo) { \
	return Tie_GetTypeInfoCount(this, MetaObject(), oNbOfTypeInfo); \
} \
HRESULT __stdcall TIE##interface##classe::GetTypeInfo(unsigned int iIndex, ULONG iLangId, ITypeInfo **oPtTypeInfo) { \
	return Tie_GetTypeInfo(this, MetaObject(), iIndex, iLangId, oPtTypeInfo); \
} \
HRESULT __stdcall TIE##interface##classe::GetIDsOfNames(const IID &forFutur, CATWCHAR_T **iArrayOfNames, unsigned int iNbNames, ULONG iLangId, LONG *oArrayOflong) { \
	return Tie_GetIDsOfNames(this, MetaObject(), forFutur, iArrayOfNames, iNbNames, iLangId, oArrayOflong); \
} \
HRESULT __stdcall TIE##interface##classe::Invoke(LONG ilongMember, const IID &forFutur,ULONG iLangId, unsigned short iFlags, DISPPARAMS *iPdisparams, VARIANT *oPvaresult, EXCEPINFO *oPexcepinfo, unsigned int *oPuArgErr) { \
	return Tie_Invoke(this, MetaObject(), ilongMember, forFutur, iLangId, iFlags, iPdisparams, oPvaresult, oPexcepinfo, oPuArgErr); \
}

/**
 * @nodoc
 * Implementation of the IDispatch methods for TIE not derivating from CATBaseDispatch.
 */
#define CATImplementIDispatchMethodsForNotCATBaseDispatchTIE(interface, classe) \
HRESULT __stdcall TIE##interface##classe::GetTypeInfoCount(unsigned int *oNbOfTypeInfo) { \
	return E_NOTIMPL; \
} \
HRESULT __stdcall TIE##interface##classe::GetTypeInfo(unsigned int iIndex, ULONG iLangId, ITypeInfo **oPtTypeInfo) { \
	return E_NOTIMPL; \
} \
HRESULT __stdcall TIE##interface##classe::GetIDsOfNames(const IID &forFutur, CATWCHAR_T **iArrayOfNames, unsigned int iNbNames, ULONG iLangId, LONG *oArrayOflong) { \
	return E_NOTIMPL; \
} \
HRESULT __stdcall TIE##interface##classe::Invoke(LONG ilongMember, const IID &forFutur,ULONG iLangId, unsigned short iFlags, DISPPARAMS *iPdisparams, VARIANT *oPvaresult, EXCEPINFO *oPexcepinfo, unsigned int *oPuArgErr) { \
	return E_NOTIMPL; \
}

/**
 * @nodoc
 * Implementation of the IDispatch methods for TIE deriving from CATBaseUnknown.
 */
#define CATImplementIDispatchMethodsForCATBaseUnknownTIE(interface, classe) \
	CATImplementIDispatchMethodsForTIE(interface, classe)

/**
 * @nodoc
 * Implementation of the IDispatch methods for TIE NOT deriving from CATBaseUnknown.
 */
#define CATImplementIDispatchMethodsForNotCATBaseUnknownTIE(interface, classe) \
	CATImplementIDispatchMethodsForTIE(interface, classe)

/**
 * @nodoc
 * Declaration of the CATBaseUnknown methods for TIE.
 */
#define CATDeclareCATBaseUnknownMethodsForTIE \
	static Exported CATMetaClass * __stdcall MetaObject(); \
	virtual CATMetaClass * __stdcall GetMetaObject() const;

/**
 * @nodoc
 * Implementation of the CATBaseUnknown methods for TIE.
 */
#define CATImplementCATBaseUnknownMethodsForTIE(interface, classe) \
CATMetaClass * __stdcall TIE##interface##classe::GetMetaObject() const { \
	return MetaObject(); \
}

/**
 * @nodoc
 * Implementation of the creation function of the TIE.
 */
#define CATImplementTIECreation(interface, classe) \
extern "C" Exported IUnknown *CreateTIE##interface##classe(CATBaseUnknown *pt = NULL, CATBaseUnknown *delegue = NULL) { \
	return new TIE##interface##classe(pt, delegue); \
}

/**
 * @nodoc
 * Implementation of the creation function of the TIEchain.
 */
#define CATImplementTIEchainCreation(interface, classe) \
extern "C" Exported IUnknown *CreateTIE##interface##classe(CATBaseUnknown *pt = NULL, CATBaseUnknown *delegue = NULL) { \
	IUnknown *ext = Tie_Link(pt, delegue, IID_##interface); \
	if (!ext) ext = new TIE##interface##classe(pt,delegue); \
	return ext; \
}

	
#endif // CATMacForTie_h
