set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdVolImaging)

pxr_library(usdVolImaging
    LIBRARIES
        usdImaging

    PUBLIC_CLASSES
        dataSourceFieldAsset
        dataSourceParticleField
        field3dAssetAdapter
        openvdbAssetAdapter
        particleFieldAdapter
        tokens

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
