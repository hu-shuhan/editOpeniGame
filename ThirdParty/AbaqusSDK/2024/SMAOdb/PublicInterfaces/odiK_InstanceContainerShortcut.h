#ifndef odiK_InstanceContainerShortcut_h
#define odiK_InstanceContainerShortcut_h

//
// Begin local includes
//
#include <odiK_InstanceContainerFwd.h>

//
// Forward declarations
//
class odiK_Assembly;

ddr_SHORTCUT_DECL(odiK_InstanceContainer, odiK_InstanceContainerShortcut);
ddr_MEMBER_SHORTCUT_DECL(odiK_Assembly, odiK_InstanceContainer, odiK_InstanceContainerInAssemblyShortcut);

#endif
