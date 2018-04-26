set(DIRS
    ar
    kind
    sdf
    pcp
    usd
    usdGeom
    usdVol
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

