/* -*- mode: c++ -*- */

#ifndef ddr_UserDataContainerShortcut_h
#define ddr_UserDataContainerShortcut_h

// Begin local includes
#include <ddr_UserDataContainerFwd.h>

// Forward declarations
class ddr_Ddb;

ddr_SHORTCUT_DECL(ddr_UserDataContainer, ddr_UserDataContainerShortcut);
ddr_MEMBER_SHORTCUT_DECL(ddr_Ddb, ddr_UserDataContainer, ddr_UserDataContainerInDdbShortcut);

#endif // #ifndef ddr_UserDataContainerShortcut_h
