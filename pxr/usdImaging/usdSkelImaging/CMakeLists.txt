set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdSkelImaging)

pxr_library(usdSkelImaging
    LIBRARIES
        hio
        hd
        usdImaging
        usdSkel

    PUBLIC_CLASSES
        animationAdapter
        animationSchema
        bindingAPIAdapter
        bindingSchema
        blendShapeAdapter
        blendShapeSchema
        dataSourceAnimationPrim
        dataSourceBindingAPI
        dataSourceBlendShapePrim
        dataSourceSkeletonPrim
        inbetweenShapeSchema
        package
        skeletonAdapter
        skeletonSchema
        skelRootAdapter
        tokens
        utils

    PUBLIC_HEADERS
        api.h
        schemaTypeDefs.h

    RESOURCE_FILES
        plugInfo.json
        shaders/skinning.glslfx
)
