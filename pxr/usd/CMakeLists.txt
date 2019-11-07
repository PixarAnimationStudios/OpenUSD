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
    usdMedia
    usdShade
    usdRender
    usdHydra
    usdRi
    usdSkel
    usdUI
    usdUtils
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

