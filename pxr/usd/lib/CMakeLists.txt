set(DIRS
    ar
    kind
    sdf
    pcp
    usd
    usdGeom
    usdShade
    usdHydra
    usdRi
    usdUI
    usdUtils
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

