set(DIRS
    ar
    kind
    sdf
    ndr
    sdr
    pcp
    usd
    usdGeom
    usdVol
    usdMedia
    usdShade
    usdLux
    usdProc
    usdRender
    usdHydra
    usdRi
    usdSkel
    usdUI
    usdUtils
    usdPhysics
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_USD_TOOLS)
    add_subdirectory(bin)
else()
    message(STATUS "Skipping commandline tools because PXR_BUILD_USD_TOOLS=OFF")
endif()

if (${PXR_ENABLE_MATERIALX_SUPPORT})
    add_subdirectory(usdMtlx)
endif()
