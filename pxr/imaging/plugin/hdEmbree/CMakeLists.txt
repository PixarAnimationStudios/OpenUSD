set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdEmbree)

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
        Embree::embree

    PRIVATE_CLASSES
        config
        debugCodes
        implicitSurfaceSceneIndexPlugin
        instancer
        light
        lightSamplers
        mesh
        meshSamplers
        pxrIES/ies
        pxrIES/pxrIES
        renderBuffer
        renderer
        rendererPlugin
        renderDelegate
        renderPass
        sampler

    PRIVATE_HEADERS
        context.h
        renderParam.h
        pxrPbrt/pbrtUtils.h

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
