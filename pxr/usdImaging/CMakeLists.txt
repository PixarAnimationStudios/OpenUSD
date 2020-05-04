set(DIRS
    usdImaging
    usdImagingGL
    usdRiImaging
    usdSkelImaging
    usdVolImaging
	  usdNprImaging
    usdAppUtils
    usdviewq
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
