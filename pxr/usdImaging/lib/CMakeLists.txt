set(DIRS
    usdImaging
    usdImagingGL
    usdShaders
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
