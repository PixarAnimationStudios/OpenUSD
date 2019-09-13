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
    usdLux
    usdShade
    usdRender
    usdHydra
    usdRi
    usdSkel
    usdUI
    usdUtils
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

