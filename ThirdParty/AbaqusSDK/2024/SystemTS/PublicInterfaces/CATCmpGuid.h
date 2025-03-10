#ifndef CATCmpGuid_h
#define CATCmpGuid_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "IUnknown.h"



/**
 * Compares two GUIDs.
 * @param iGuid1
 * the first <tt>GUID</tt>.
 * @param iGuid2
 * the second <tt>GUID</tt>.
 * @return
 * <tt>0</tt> if the two <tt>GUIDs</tt> are different, not <tt>0</tt> otherwise.
 */
inline int CATCmpGuid(const GUID  *iGuid1, const GUID *iGuid2);

/* @nodoc **/
inline int CATCmpGuid(const GUID  *iGuid1, const GUID *iGuid2) {
   return (
      ((int*) iGuid1)[0] == ((int*) iGuid2)[0] &&
      ((int*) iGuid1)[1] == ((int*) iGuid2)[1] &&
      ((int*) iGuid1)[2] == ((int*) iGuid2)[2] &&
      ((int*) iGuid1)[3] == ((int*) iGuid2)[3]);
}

#endif
