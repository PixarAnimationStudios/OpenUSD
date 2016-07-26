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
    usdUtils
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

