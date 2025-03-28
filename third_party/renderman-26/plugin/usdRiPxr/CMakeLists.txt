set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE usdRiPxr)

# Conditional logic based on RenderMan version
if (RENDERMAN_VERSION VERSION_LESS "26.2")
    pxr_library(usdRiPxr
        RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdRiPxr/schema.usda
    )
else()
    # Get the usdRiPxr plugin files from renderman itself
    # usdRiPxr schemas are known to be installed under the following path:
    # ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources
    pxr_library(usdRiPxr
        RESOURCE_FILES
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/plugInfo.json:plugInfo.json
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/generatedSchema.usda:generatedSchema.usda
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/usdRiPxr/schema.usda:usdRiPxr/schema.usda
    )
endif()
