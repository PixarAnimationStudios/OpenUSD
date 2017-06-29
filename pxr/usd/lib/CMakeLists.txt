set(DIRS
    ar
    kind
    sdf
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

