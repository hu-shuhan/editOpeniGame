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
// File Name: nex_assert.h
// 
// Author: Uttam Narsu
// 
// Creation date: Mon Jun  9 1997
//
// Purpose:
//
// This file contains the definition of the nex_assert class.
//

#ifndef nex_assert_h
#define nex_assert_h

#include <assert.h>

// Begin local includes
#include <nex_ExceptionHandling.h>
#include <nex_AssertException.h>

// The nex_assertFormalize function is defined to maintain a dependence
// on the nex facility when NDEBUG is not defined, i.e., when the
// assert macro is enabled. This function always returns false. But the
// nex_assert macro is constructed such that this function will never
// be called.


bool nex_assertFormalize();

#if !defined(NDEBUG)

#define nex_assert(cond) ((cond)?void():assert(cond),nex_assertFormalize())

#else

#define nex_assert(cond) ((cond)?void():HKS_THROW(nex_AssertException(# cond)))

#endif

#endif  // #ifdef nex_assert_h
