set(PXR_PREFIX "")
set(PXR_PACKAGE hdxPrman)

# TODO The pxr cmake macros don't provide a good way to link prman with link_directories
# on Linux/OSX without resorting to absolute rpath. For now, simply avoid linking
# libprman on Linux/OSX.
set(optionalLibs "")
if(WIN32)
    list(APPEND optionalLibs ${PXRCORE_LIBRARY})
    list(APPEND optionalLibs ${PRMAN_LIBRARY})
elseif(APPLE)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -headerpad_max_install_names" )
endif()
pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        hd
        work
        ${optionalLibs}
        hdPrman

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PRIVATE_CLASSES
        context
        framebuffer
        renderBuffer
        renderDelegate
        renderPass
        resourceRegistry

    CPPFILES
        rendererPlugin

    PRIVATE_HEADERS
        renderParam.h

    RESOURCE_FILES
        plugInfo.json
)

# Includes for hdPrman headers in this library look like
# #include "hdPrman/foo.h", so we need to add hdPrman's parent 
# include directory to the include paths for this library.
#
# Make sure to check if the target exists first, otherwise this
# will error out in cases (e.g. building static libraries) where
# the plugin is not build.
if (TARGET ${PXR_PACKAGE})
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${CMAKE_BINARY_DIR}/include/pxr/imaging"
    )
endif()
