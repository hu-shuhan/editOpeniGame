#ifndef odiK_AssemblyContainerShortcut_h
#define odiK_AssemblyContainerShortcut_h

//
// Begin local includes
//
#include <odiK_AssemblyContainerFwd.h>

//
// Forward declarations
//
class odiK_Model;

ddr_SHORTCUT_DECL(odiK_AssemblyContainer, odiK_AssemblyContainerShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_Model, odiK_AssemblyContainer, odiK_AssemblyContainerInModelShortcut);

#endif
