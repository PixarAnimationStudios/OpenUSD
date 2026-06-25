#
# Copyright 2025 Apple
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

find_library(PMC_LIBRARY NAMES pmc)
find_path(PMC_INCLUDES pmc/pmDecoder.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pmc
    REQUIRED_VARS PMC_LIBRARY PMC_INCLUDES
)

if(PMC_LIBRARY AND NOT TARGET Pmc::Pmc)
    add_library(Pmc::Pmc STATIC IMPORTED)
    set_target_properties(Pmc::Pmc PROPERTIES
        IMPORTED_LOCATION "${PMC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PMC_INCLUDES}"
        POSITION_INDEPENDENT_CODE ON
    )
endif()
