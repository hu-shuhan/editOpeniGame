//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: nex_Exception.h
// 
// Author: Uttam Narsu
// 
// Creation date: Tue Mar  9 1999
//
// Purpose:
//
// This file contains the definition of the nex_Exception base class.
//

#ifndef nex_Exception_h
#define nex_Exception_h

//
// Includes
//

// Begin local includes
#include <mem_AllocationOperators.h>
#include <SMABasException.h>

#include <cow_COW.h>
#include <atr_String.h>
#include <cow_String.h>
#include <typ_typeTag.h>

#include <nex_ExceptionHandling.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>
#include <SMABasStringMode.h>

//
// Constants
//

// Macros for use in type tables.
//    This macro goes in the <fac>_TypeTable.C file.
//
#define NEX_EXCEPTION_TT(NAME, BASE)\
    {\
        typ_ExceptionTT,\
        TYP_TIX_ ## BASE,\
        #NAME\
    }

// makes the above macro usable for the base class (nex_Exception) itself
#define TYP_TIX_typ_NONE typ_NONE

// Sometimes we had ODT exceptions in the same fac as other
// non-ODT classes.  The database would get changed if we changed
// the TYP_TIX values, but leaving them the same, means that you
// need to have blank entries in the fac_TypeTable.C file. The
// following macro inserts a blank. The arg, n, is just in case
// you want to keep track of the numbers, eg. NEX_BLANK_TT(0xa)
#define NEX_BLANK_TT(n)\
    {\
        typ_ErrorTT,\
        typ_NONE,\
        NULL\
    }


//
// Class definition
//

class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;

class nex_Exception  : public SMABasException
{
public:
    virtual ~nex_Exception();

    virtual atr_String AsString(STRMODE_DECL) const = 0;

    // These two are needed in the IPC so that we can throw
    // an exception received across the IPC
    virtual void Propagate() const = 0;
    virtual nex_Exception* Copy() const = 0;

    // Messaging/database interface
    virtual typ_typeTag     DynTypeId() const;
    static typ_typeTag      TypeId();

protected:
    nex_Exception()
    { }

public:

    // Messaging/Database interface
    nex_Exception( const cls_ReadVisitor & );
    static void CowDtor( cls_IntCOW * );
    virtual void DBWrite( const cls_WriteVisitor & ) const;
    static void InitializeMetadata( cls_ClassRegistrar & );

    virtual int GetCode() const;

private:
    cls_Uid m_ClsUid;
};

// For those classes (cmdC_Reply,pyt_ExceptionValue) that need to
// hang onto exception objects
COW_COW2_DECL(nex_Exception,cow_Virtual);


#endif  // #ifdef nex_Exception_h
