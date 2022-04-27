set(DIRS
    ar
    kind
    sdf
    ndr
    sdr
    exec
    pcp
    usd
    usdGeom
    usdVol
    usdMedia
    usdShade
    usdLux
    usdRender
    usdHydra
    usdRi
    usdSkel
    usdNpr
    usdUI
    usdUtils
    usdPhysics
	usdAnimX
    usdExec
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
