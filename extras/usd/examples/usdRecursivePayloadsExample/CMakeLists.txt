set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRecursivePayloadsExample)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        gf
        sdf
        usd

    CPPFILES
        fileFormat.cpp

    RESOURCE_FILES
        plugInfo.json
        root.usda:usdRecursivePayloadsExample/root.usda
        params.usda:usdRecursivePayloadsExample/params.usda
        payload.usda:usdRecursivePayloadsExample/payload.usda
        cone.usdrecursivepayloadsexample:usdRecursivePayloadsExample/cone.usdrecursivepayloadsexample
        sphere.usdrecursivepayloadsexample:usdRecursivePayloadsExample/sphere.usdrecursivepayloadsexample

    DISABLE_PRECOMPILED_HEADERS
)

