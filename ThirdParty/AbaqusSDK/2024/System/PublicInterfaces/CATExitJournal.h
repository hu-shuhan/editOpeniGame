#ifndef CATExitJournal_h
#define CATExitJournal_h


// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */

#include "CATBaseUnknown.h"
#include "JS0LOGRP.h"
//#include "CATICallTrac2.h"
class CATICallTrac2;

extern "C" ExportedByJS0LOGRP void ExitAfterCall(const CATBaseUnknown * const objthis, long nummethod, CATICallTrac2 * journcour, ...);

extern "C"  ExportedByJS0LOGRP void ExitBeforeCall(const CATBaseUnknown * const objthis, long nummethod, CATICallTrac2 ** journcour, ...);

#endif
