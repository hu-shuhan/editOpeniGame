#ifndef CATMacForTie_h  // same guard macro as "CATMacForTie.h"
// Don't define the macro here, unless we need to override the official header
// COPYRIGHT DASSAULT SYSTEMES 2023
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/** @nodoc */
#define CATDeclareCommonTIEMembers2 CATDeclareCommonTIEMembers

/** @nodoc */
#define CATDefineCommonTIEMembers2(interface, classe) CATDefineCommonTIEMembers(interface, classe)

/** @nodoc */
#define CATImplementTIEDestructor(interface, classe) \
TIE##interface##classe::~TIE##interface##classe() { \
	Tie_Destruct(this, &NecessaryData.ForTIE, MetaObject()->GetTypeOfClass(), m_cRef); \
}

#endif
