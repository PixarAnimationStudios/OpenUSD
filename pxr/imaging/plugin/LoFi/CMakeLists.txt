set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE LoFi)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_plugin(LoFi
   LIBRARIES
        plug
        tf
        vt
        gf
        work
        hf
        hd
        hdx
        ${TBB_tbb_LIBRARY}
        ${EMBREE_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        ${EMBREE_INCLUDE_DIR}

    PUBLIC_CLASSES
        debugCodes
        tokens
        shader
        vertexBuffer
        vertexArray
        drawItem
        resourceRegistry
        utils
        mesh
        sceneDelegate
        renderDelegate
        renderPass
        rendererPlugin
        
    PUBLIC_HEADERS
        api.h
        timer.h
        shader.h
        renderParam.h

    RESOURCE_FILES
        plugInfo.json
)

if (X11_FOUND OR APPLE)
pxr_build_test(testLoFi
    LIBRARIES
        LoFi
        garch
        arch
    CPPFILES
        testenv/testLoFi.cpp
)
endif()
