set(DIRS
    usdImaging
    usdImagingGL
    usdShaders
    usdSkelImaging
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
