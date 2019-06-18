set(PXR_PREFIX "")
set(PXR_PACKAGE hdxPrman)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        hdx
        work
        hdPrman
        ${GLEW_LIBRARY}
        ${RENDERMAN_LIBRARY}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PRIVATE_CLASSES
        context
        framebuffer
        renderBuffer
        renderDelegate
        renderPass
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
