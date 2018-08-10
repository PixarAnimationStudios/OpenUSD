set(DIRS
    ar
    kind
    sdf
    ndr
    sdr
    pcp
    usd
    usdGeom
    usdLux
    usdShade
    usdHydra
    usdRi
    usdSkel
    usdUI
    usdUtils
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

