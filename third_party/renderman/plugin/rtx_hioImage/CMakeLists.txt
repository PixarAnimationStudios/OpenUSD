set(PXR_PREFIX "")
set(PXR_PACKAGE RtxHioImage)

pxr_plugin(${PXR_PACKAGE}

    LIBRARIES
        tf
        gf
        trace
        hio
        ${PXRCORE_LIBRARY}
        ${PRMAN_LIBRARY}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    CPPFILES
        main.cpp 

    PRIVATE_HEADERS

    RESOURCE_FILES

    DISABLE_PRECOMPILED_HEADERS
)

if(TARGET RtxHioImage)
    if(WIN32)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE _WIN32)
    elseif(APPLE)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE OSX)
    elseif(UNIX)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE LINUX)
    endif()
endif()
