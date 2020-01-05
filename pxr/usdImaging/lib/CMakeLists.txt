set(DIRS
    usdImaging
    usdImagingGL
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
