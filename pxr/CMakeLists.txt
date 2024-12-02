pxr_core_prologue()

add_subdirectory(external)
add_subdirectory(base)
add_subdirectory(usd)

if (EXISTS "${PROJECT_SOURCE_DIR}/pxr/exec")
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

configure_file(pxrConfig.cmake.in
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

install(EXPORT pxrTargets DESTINATION "cmake")
