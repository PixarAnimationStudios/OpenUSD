set(DIRS
    usdValidation
    usdGeomValidators
    usdPhysicsValidators
    usdShadeValidators
    usdSkelValidators
    usdUtilsValidators
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_USD_TOOLS)
    add_subdirectory(bin)
else()
    message(STATUS "Skipping commandline tools because PXR_BUILD_USD_TOOLS=OFF")
endif()

