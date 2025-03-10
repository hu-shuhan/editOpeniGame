#ifndef CATERRDSC_INCLUDE
#define CATERRDSC_INCLUDE
// Copyright Dassault Systemes 1996

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

//-----------------------------------------------------------------------------
// Abstract:	The structures holding error messages
//-----------------------------------------------------------------------------
// Usage:	This how error messages are stored at run-time. Only CATError
//		and derived classes access these structures.
//-----------------------------------------------------------------------------
// Note:	The implementation code is separated into 2 files:
//		CATErrDsc.cpp for the methods used at run-time
//		CATErrDscParse.cpp for the methods used at parse-time
//-----------------------------------------------------------------------------
#include <stdio.h>
#include "CATErrParams.h"
#include "CATErrorDefs.h"

/** @nodoc */
#define CATERRDSC_MAX_PARAMS		(CATERROR_MAX_INT_PARAMS+\
					 CATERROR_MAX_REAL_PARAMS+\
					 CATERROR_MAX_CHARS_PARAMS)

/** @nodoc */
#define CATERRDSC_MAX_PARTS		(((CATERRDSC_MAX_PARAMS+1) \
					 + 7) & 0x7ffffff8)

// Number of messages for each error
/** @nodoc */
#define CATERRDSC_SHORT_MESSAGE		0
/** @nodoc */
#define CATERRDSC_LONG_MESSAGE		1
/** @nodoc */
#define CATERRDSC_MAX_MESSAGES		2

//
// Error Message class: knows how to parse itself from external to internal
//                      forms and to format using the parameters
//
/** @nodoc */
class CATErrMsg {

  public:
    /** @nodoc */
    const char *text;			// pointer to all the text parts
    /** @nodoc */
    unsigned int numParts;		// number of parts in the message
    /** @nodoc */
    unsigned char parts[CATERRDSC_MAX_PARTS]; // description of each part

    //
    // Parse time methods
    //
    // Store a new parameter that has been parsed
    /** @nodoc */
    int AddParam (char type, int index);
    // Store a new text part
    /** @nodoc */
    int AddText (const char *start, const char *end);
    // Output a  message in C/C++ format to be use as an initializer
    /** @nodoc */
    void OutputInitializer (FILE *out);
    // Parse a message  in msg format
    /** @nodoc */
    int Parse (const char *msgstr);
    //
    // Run time methods
    //
    // Format a message with given parameters
    /** @nodoc */
    const char *Format (const CATErrParams *) const;

};

//
// Structure for each error
// 
// This class will be initialized using initializer lists and thus cannot
// have private members, constructor, destructor and the like.
//
/**
 * Do not use this structure. It is used internally by CATError software.
 * <p><b>Role</b>:  Do not use.
 *  Only CATError class should access these structure.
 */
class CATErrDsc {

  public:
    /** @nodoc */
    void SetId (CATErrorId x) { this->id = x; }
    /** @nodoc */
    CATErrorId GetId () const { return id; }
    //
    // Parse-time methods
    //
    // Called to parse each message
    /** @nodoc */
    int  ParseMessage (int whichMessage, char *message);
    // Output C/C++ initializer
    /** @nodoc */
    void OutputInitializer (FILE *out);
    //
    // Run-time methods
    // Format a message
    /** @nodoc */
    const char *Format (int whichMessage, const CATErrParams *) const;
    // Compare function
    /** @nodoc */
    static int Compare (const CATErrDsc *e1, const CATErrDsc *e2);

    /** @nodoc */
    CATErrorId id;
    /** @nodoc */
    const char *name;
    /** @nodoc */
    CATErrMsg messages[CATERRDSC_MAX_MESSAGES];
};

// Compare macro
/** @nodoc */
#define CATERRDSC_COMPARE(x1,x2)	((x1)->id - (x2)->id)

/** @nodoc */
typedef enum {
/** @nodoc */
    CATErrDscInformation,
/** @nodoc */
    CATErrDscWarning,
/** @nodoc */
    CATErrDscError,
/** @nodoc */
    CATErrDscFatal
} CATErrDscSeverity;


#endif
