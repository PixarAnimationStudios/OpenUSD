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
        shaderCode
        binding
        codeGen
        shader
        topology
        vertexBuffer
        vertexArray
        drawItem
        resourceRegistry
        utils
        halfedge
        mesh
        points
        instancer
        sceneDelegate
        renderDelegate
        renderPass
        rendererPlugin
        
    PUBLIC_HEADERS
        api.h
        timer.h
        codeGen.h
        shader.h
        renderParam.h

    RESOURCE_FILES
        plugInfo.json
        shaders/common.glslfx
        shaders/simple.glslfx
        shaders/mesh.glslfx
        shaders/points.glslfx
        shaders/visibility.glslfx
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
