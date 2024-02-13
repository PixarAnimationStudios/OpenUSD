set(DIRS
    usdImaging
    usdImagingGL
    usdProcImaging
    usdRiPxrImaging
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
    plugin
    bin
)

if(PXR_ENABLE_JS_BINDINGS_SUPPORT)
    list(APPEND DIRS hdEmscripten)
endif()

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
