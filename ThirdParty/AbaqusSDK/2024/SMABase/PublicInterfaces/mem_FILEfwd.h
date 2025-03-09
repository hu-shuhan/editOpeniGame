//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
/*This header file allows us to use FILE* in ABAQUS header files without 
 *including the stdio.h system header. This cuts down on compile time.
 *
 *In the implementation .C file one must include stdio.h prior to including
 *any ABAQUS header files.
 *
 *This file is a dupliction of aio_FILEfwd.h and is used to break mem->aio
 *dependency.
 */
#ifndef MEM_FILEFWD_H
#define MEM_FILEFWD_H

#ifndef EOF
#define EOF -1
#define FILE void
#endif

#endif /* MEM_FILEFWD_H */
