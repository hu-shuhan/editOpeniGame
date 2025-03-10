/* -*- mode: c++ -*- */

#ifndef ddr_CustomDataContainerShortcut_h
#define ddr_CustomDataContainerShortcut_h

// Begin local includes
#include <ddr_CustomDataContainerFwd.h>

// Forward declarations
class ddr_Ddb;

ddr_SHORTCUT_DECL(ddr_CustomDataContainer, ddr_CustomDataContainerShortcut);
ddr_MEMBER_SHORTCUT_DECL(ddr_Ddb, ddr_CustomDataContainer, ddr_CustomDataContainerInDdbShortcut);

#endif // #ifndef ddr_CustomDataContainerShortcut_h
