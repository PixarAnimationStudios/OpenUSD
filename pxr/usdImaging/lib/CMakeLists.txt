set(DIRS
    usdImaging
    usdImagingGL
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
