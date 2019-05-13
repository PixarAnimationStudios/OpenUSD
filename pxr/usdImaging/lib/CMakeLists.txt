set(DIRS
    usdAppUtils
    usdImaging
    usdImagingGL
    usdShaders
    usdSkelImaging
    usdVolImaging
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
