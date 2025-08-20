pxr_core_prologue()

add_subdirectory(external)
add_subdirectory(base)
add_subdirectory(usd)

if (${PXR_BUILD_EXEC})
    add_subdirectory(exec)
endif()

if (${PXR_BUILD_USD_VALIDATION})
    add_subdirectory(usdValidation)
endif()

if (${PXR_BUILD_IMAGING})
    add_subdirectory(imaging)
    if (${PXR_BUILD_USD_IMAGING})
        add_subdirectory(usdImaging)
    endif()
endif()

pxr_core_epilogue()

export(PACKAGE pxr)

# XXX:
# Libraries specify the TBB::tbb target to link against TBB. This target
# is either imported from the package config file that is installed with
# oneTBB (and is optionally installed with TBB), or it is created by the
# USD build as part of the FindTBB cmake module.
#
# The TBB::tbb target must be available when a client builds against USD
# using its pxrConfig.cmake package config file. In the former case above,
# clients can use the same package config file from TBB they used when
# building USD itself. But in the latter case, USD itself needs to export
# the TBB::tbb target it created.
#
# Typically a target would be exported using `export(TARGETS ...)` but
# this doesn't work for TBB::tbb because it's an imported target. So we
# manually set up this target in pxrConfig.cmake and copy over the
# relevant properties that are set up in FindTBB.cmake. These properties
# specify the TBB library that USD was actually built against and don't
# include other configurations.
#
# This can all go away when support for TBB is dropped in favor of oneTBB
# and we solely rely on its package config file.
foreach(property IN ITEMS
    INTERFACE_INCLUDE_DIRECTORIES
    INTERFACE_COMPILE_DEFINITIONS
    INTERFACE_LINK_DIRECTORIES
    IMPORTED_IMPLIB
    IMPORTED_LOCATION)

    get_target_property(value TBB::tbb ${property})
    if (value)
        set(tbb_${property} "${value}")
    endif()
endforeach()

configure_file(pxrConfig.cmake.in
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

install(EXPORT pxrTargets DESTINATION "cmake")
