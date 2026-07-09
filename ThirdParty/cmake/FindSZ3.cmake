include(FindPackageHandleStandardArgs)

if (TARGET SZ3 AND NOT TARGET SZ3::SZ3)
    add_library(SZ3::SZ3 ALIAS SZ3)
endif ()

set(SZ3_TARGET_AVAILABLE FALSE)
if (TARGET SZ3::SZ3)
    set(SZ3_TARGET_AVAILABLE TRUE)
endif ()

find_package_handle_standard_args(SZ3
        REQUIRED_VARS SZ3_TARGET_AVAILABLE)
