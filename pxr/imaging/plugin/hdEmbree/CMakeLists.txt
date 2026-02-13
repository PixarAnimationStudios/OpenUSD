set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdEmbree)

if(NOT EMBREE_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Embree")
    return()
endif()
if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

pxr_plugin(hdEmbree
   LIBRARIES
        tf
        vt
        gf
        work
        hd
        hdsi
        hio
        ${EMBREE_LIBRARY}

    INCLUDE_DIRS
        ${EMBREE_INCLUDE_DIR}

    PUBLIC_CLASSES
        config
        instancer
        light
        lightSamplers
        mesh
        meshSamplers
        pxrIES/ies
        renderBuffer
        renderer
        rendererPlugin
        renderDelegate
        renderPass
        sampler

    PUBLIC_HEADERS
        context.h
        renderParam.h
        pxrPbrt/pbrtUtils.h

    PRIVATE_CLASSES
        debugCodes
        implicitSurfaceSceneIndexPlugin

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

if (X11_FOUND OR APPLE)
pxr_build_test(testHdEmbree
    LIBRARIES
        arch
        garch
        hdEmbree
        hdSt
        hdx
        hio
    CPPFILES
        testenv/testHdEmbree.cpp
)
endif()
