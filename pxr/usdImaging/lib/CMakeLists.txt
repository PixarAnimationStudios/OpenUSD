set(DIRS
    usdImaging
    usdImagingGL
    usdSkelImaging
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
