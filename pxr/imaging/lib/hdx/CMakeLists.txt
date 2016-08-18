set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdx)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(hdx
    LIBRARIES
        plug
        tf
        vt
        gf
        work
        glf
        pxOsd
        hd
        cameraUtil
        sdf
        ${GLEW_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}

    PUBLIC_CLASSES
        debugCodes
        drawTargetTask
        intersector
        package
        renderTask
        renderSetupTask
        selectionTask
        selectionTracker
        shadowTask
        simpleLightTask
        simpleLightBypassTask
        tokens
        types

    PUBLIC_HEADERS
        shadowMatrixComputation.h
        version.h
        api.h

    PRIVATE_CLASSES
        unitTestDelegate

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}

    RESOURCE_FILES
        plugInfo.json
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/renderPassIdShader.glslfx
)
