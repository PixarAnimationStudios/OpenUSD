set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdx)

pxr_shared_library(hdx
    LIBRARIES
        plug
        tf
        vt
        gf
        work
        garch
        glf
        pxOsd
        hd
        cameraUtil
        sdf
        ${GLEW_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}

    PUBLIC_CLASSES
        camera
        debugCodes
        drawTarget
        drawTargetAttachmentDesc
        drawTargetAttachmentDescArray
        drawTargetRenderPass
        drawTargetRenderPassState
        drawTargetTask
        drawTargetResolveTask
        intersector
        light
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
        drawTargetTextureResource
        unitTestDelegate

    RESOURCE_FILES
        plugInfo.json
        shaders/simpleLightingShader.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/renderPassIdShader.glslfx
        shaders/renderPassShadowShader.glslfx
)
