//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasException_h
#define SMABasException_h

///////////////////////////////////////////////////////////////////////////////
//
// File Name: SMABasException.h
// 
// Author: Rich Simpson
// 
// Creation date: 29 August 2007
//
// Purpose:
//         SMABasException is the base of all exceptions thrown within
//         SIMULIA shared code. They can also be caught outside
//         of shared code by clients.
//         This is a virtual base class that can only be created
//         by a subclass instance.
//
//         Note that any constructors must be no-fail.
//
// NOTE: Exception SubClass Creation
//        Any SubClass of this base class must be created using
//        using code generation. The exception is defined in a .exc
//        file. This is then used to drive generation of a .h and
//        a .C file.

//
// Includes
//

// Begin local includes
#include <mem_AllocationOperators.h>
#include <atr_String.h>
#include <SMABasStringMode.h>
// End local includes

class SMABasException  : public mem_AllocationOperators
{
public:
    // The copy constructor and the assignment operator
    // typically use the default semantics. 
    // Be sure to provide override implementations if
    // the default copy semantics are not sufficient.

    // The destructor must be no-fail
    virtual ~SMABasException();

    // default impl is UserReport + file & line no
    virtual atr_String DeveloperReport(STRMODE_DECL) const;

    // default impl is to just call Xdata::AsString()
    virtual atr_String UserReport(STRMODE_DECL) const;

    virtual atr_String AsString(STRMODE_DECL) const = 0;

    // The implementation for Throw will record the
    // location info and propagate the exception. 
    void Throw(const char * fname, int lineNum);

    virtual void Propagate() const = 0;

    virtual SMABasException* Clone() const { return 0; }

    virtual int GetCode() const = 0;

    int GetFromLine() const {
        return m_FromLine;
    }

    const char * GetFromFile() const {
        return m_FromFile;
    }

protected:
    SMABasException()
	: m_FromFile(), m_FromLine(0)
    { }

    SMABasException(const char * fromFile, int fromLine)
	: m_FromFile(fromFile), m_FromLine(fromLine)
    { }

    const char *  m_FromFile;  // filename where exception was raised
    int           m_FromLine;  // line num where exception was raised

};

// These macros exist to ensure consistent behavior including
// setting of the line numbers and throw by value/catch by reference.
#define SMABAS_THROW(REF) REF.Throw(__FILE__, __LINE__)
#define SMABAS_TRY try
#define SMABAS_CATCH(CLASS, REF) catch (const CLASS & REF)
#define SMABAS_CATCHTYPE(CLASS) catch (const CLASS &)
#define SMABAS_CATCHALL catch (...)

#endif  // SMABasException_h
