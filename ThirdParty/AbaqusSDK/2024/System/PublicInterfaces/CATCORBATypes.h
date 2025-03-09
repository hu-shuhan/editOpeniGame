#ifndef _CATCORBATypes_h
#define _CATCORBATypes_h

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATCORBABoolean.h"
#include "JS0CTYP.h"

/** @nodoc */
typedef struct ExportedByJS0CTYP stroct {
	unsigned char  	valoct;
	stroct();
	stroct(char);
	operator char() const;
	stroct(long);
	operator long () const;
	stroct(unsigned long);
	operator unsigned long () const;
	stroct(short);
	operator short () const;
	stroct(unsigned short);
	operator unsigned short() const;
	stroct(int);
	operator int () const;
	stroct(unsigned int);
	operator unsigned int() const;
} overoctet;

/** octet for CORBA type. */
typedef unsigned char octet;


#endif








