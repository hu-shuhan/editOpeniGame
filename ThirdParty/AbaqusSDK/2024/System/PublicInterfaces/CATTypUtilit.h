
#ifndef CATTypUtilit_h
#define CATTypUtilit_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATCORBATypes.h"
#include "JS0CORBA.h"

#include "CATCORBASequence.h"

#include "CATCORBABoolean.h"

#include "CATCORBAAny.h"

#include "CATBaseUnknown.h"


DEF_SEQ(_SEQUENCE_SEQUENCE_any, any);

typedef  _SEQUENCE_SEQUENCE_any  SEQUENCE_any;

DEF_SEQ(_SEQUENCE_SEQUENCE_octet, octet);

typedef  _SEQUENCE_SEQUENCE_octet  SEQUENCE_octet;

DEF_SEQ(_SEQUENCE_SEQUENCE_boolean, boolean);

typedef  _SEQUENCE_SEQUENCE_boolean  SEQUENCE_boolean;

DEF_SEQ(_SEQUENCE_SEQUENCE_ushort, unsigned short);

typedef  _SEQUENCE_SEQUENCE_ushort  SEQUENCE_ushort;

DEF_SEQ(_SEQUENCE_SEQUENCE_string, char *);

typedef  _SEQUENCE_SEQUENCE_string  SEQUENCE_string;

DEF_SEQ(_SEQUENCE_SEQUENCE_objet, CATBaseUnknown);

typedef  _SEQUENCE_SEQUENCE_objet  SEQUENCE_objet;


#endif
