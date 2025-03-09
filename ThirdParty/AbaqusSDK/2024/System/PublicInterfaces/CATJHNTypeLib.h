#ifndef CATDescTypeLib_h
#define CATDescTypeLib_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

enum TYPEKIND{
	TKIND_ENUM,
	TKIND_RECORD,
  TKIND_MODULE,
  TKIND_INTERFACE,
  TKIND_DISPATCH,
  TKIND_COCLASS,
  TKIND_ALIAS,
  TKIND_UNION,
  TKIND_MAX
};

typedef unsigned short VARTYPE;

/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 *
 *
 *  VT_EMPTY            [V]   [P]     nothing
 *  VT_NULL             [V]   [P]     SQL style Null
 *  VT_I2               [V][T][P][S]  2 byte signed int
 *  VT_I4               [V][T][P][S]  4 byte signed int
 *  VT_R4               [V][T][P][S]  4 byte real
 *  VT_R8               [V][T][P][S]  8 byte real
 *  VT_CY               [V][T][P][S]  currency
 *  VT_DATE             [V][T][P][S]  date
 *  VT_BSTR             [V][T][P][S]  OLE Automation string
 *  VT_DISPATCH         [V][T][P][S]  IDispatch *
 *  VT_ERROR            [V][T][P][S]  SCODE
 *  VT_BOOL             [V][T][P][S]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]  VARIANT *
 *  VT_UNKNOWN          [V][T]   [S]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S]  16 byte fixed point
 *  VT_I1                  [T]        signed char
 *  VT_UI1              [V][T][P][S]  unsigned char
 *  VT_UI2                 [T][P]     unsigned short
 *  VT_UI4                 [T][P]     unsigned short
 *  VT_I8                  [T][P]     signed 64-bit int
 *  VT_UI8                 [T][P]     unsigned 64-bit int
 *  VT_INT                 [T]        signed machine int
 *  VT_UINT                [T]        unsigned machine int
 *  VT_VOID                [T]        C style void
 *  VT_HRESULT             [T]        Standard return type
 *  VT_PTR                 [T]        pointer type
 *  VT_SAFEARRAY           [T]        (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]        C style array
 *  VT_USERDEFINED         [T]        user defined type
 *  VT_LPSTR               [T][P]     null terminated string
 *  VT_LPWSTR              [T][P]     wide null terminated string
 *  VT_FILETIME               [P]     FILETIME
 *  VT_BLOB                   [P]     Length prefixed bytes
 *  VT_STREAM                 [P]     Name of the stream follows
 *  VT_STORAGE                [P]     Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]     Stream contains an object
 *  VT_STORED_OBJECT          [P]     Storage contains an object
 *  VT_BLOB_OBJECT            [P]     Blob contains an object
 *  VT_CF                     [P]     Clipboard format
 *  VT_CLSID                  [P]     A Class ID
 *  VT_VECTOR                 [P]     simple counted array
 *  VT_ARRAY            [V]           SAFEARRAY*
 *  VT_BYREF            [V]           void* for local use
 *  VT_BSTR_BLOB                      Reserved for system use
 */

enum VARENUM {
  VT_EMPTY	= 0,
  VT_NULL	= 1,
  VT_I2	= 2,
  VT_I4	= 3,
  VT_R4	= 4,
  VT_R8	= 5,
  VT_CY	= 6,
  VT_DATE	= 7,
  VT_BSTR	= 8,
  VT_DISPATCH	= 9,
  VT_ERROR	= 10,
  VT_BOOL	= 11,
  VT_VARIANT	= 12,
  VT_UNKNOWN	= 13,
  VT_DECIMAL	= 14,
  VT_I1	= 16,
  VT_UI1	= 17,
  VT_UI2	= 18,
  VT_UI4	= 19,
  VT_I8	= 20,
  VT_UI8	= 21,
  VT_INT	= 22,
  VT_UINT	= 23,
  VT_VOID	= 24,
  VT_HRESULT	= 25,
  VT_PTR	= 26,
  VT_SAFEARRAY	= 27,
  VT_CARRAY	= 28,
  VT_USERDEFINED	= 29,
  VT_LPSTR	= 30,
  VT_LPWSTR	= 31,
  VT_FILETIME	= 64,
  VT_BLOB	= 65,
  VT_STREAM	= 66,
  VT_STORAGE	= 67,
  VT_STREAMED_OBJECT	= 68,
  VT_STORED_OBJECT	= 69,
  VT_BLOB_OBJECT	= 70,
  VT_CF	= 71,
  VT_CLSID	= 72,
  VT_BSTR_BLOB	= 0xfff,
  VT_VECTOR	= 0x1000,
  VT_ARRAY	= 0x2000,
  VT_BYREF	= 0x4000,
  VT_RESERVED	= 0x8000,
  VT_ILLEGAL	= 0xffff,
  VT_ILLEGALMASKED	= 0xfff,
  VT_TYPEMASK	= 0xfff
};


#else /* _WINDOWS_SOURCE */
#include "oaidl.h"
#endif
#ifndef CORBA_MERGE_TYPE
#define CORBA_MERGE_TYPE

    enum TCKind {
	    tk_null,	    tk_void,	tk_short,
	    tk_long,	    tk_ushort,	tk_ulong,
	    tk_float,	    tk_double,	tk_boolean,
	    tk_char,	    tk_octet,   tk_any,
	    tk_TypeCode,    tk_Principal,		 
	    tk_objref,  tk_struct,      tk_union,
	    tk_enum,    tk_string,      tk_sequence,
	    tk_array,	tk_alias,	tk_except

		/* DS Additions */
		, tk_specobject, tk_external, tk_list, tk_component, tk_error
		, tk_integer, tk_techno

	};

#endif // ifndef CORBA_MERGE_TYPE


#endif
