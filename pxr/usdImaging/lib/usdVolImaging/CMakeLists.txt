set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdVolImaging)

pxr_library(usdVolImaging
    LIBRARIES
        usdImaging

    PUBLIC_CLASSES
        openvdbAssetAdapter
        field3dAssetAdapter

    RESOURCE_FILES
        plugInfo.json
)
