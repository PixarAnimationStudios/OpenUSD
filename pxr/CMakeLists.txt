pxr_core_prologue()

add_subdirectory(base)
add_subdirectory(usd)

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

if (NOT PXR_BUILD_MONOLITHIC)
    install(EXPORT pxrTargets DESTINATION "cmake")
endif()
