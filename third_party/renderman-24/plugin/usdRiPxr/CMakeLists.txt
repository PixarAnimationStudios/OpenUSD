set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE usdRiPxr)

pxr_library(usdRiPxr
    RESOURCE_FILES
    plugInfo.json
    generatedSchema.usda
    schema.usda:usdRiPxr/schema.usda
)
