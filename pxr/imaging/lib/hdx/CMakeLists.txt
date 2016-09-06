set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdx)

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
        simpleLightingShader
        simpleLightTask
        simpleLightBypassTask
        tokens
        types

    PUBLIC_HEADERS
        shadowMatrixComputation.h
        version.h

    PRIVATE_CLASSES
        unitTestDelegate

    RESOURCE_FILES
        plugInfo.json
        shaders/simpleLightingShader.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/renderPassIdShader.glslfx
)
