set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE usdRiPxr)

# Get the usdRiPxr plugin files from RenderMan.
# usdRiPxr schemas are known to be installed under the following path:
# ${RENDERMAN_LOCATION}/lib/usd/schemas/usdRiPxr/resources
if (EXISTS "${RENDERMAN_LOCATION}/lib/usd/schemas/usdRiPxr/resources")
    pxr_library(usdRiPxr
        RESOURCE_FILES
        ${RENDERMAN_LOCATION}/lib/usd/schemas/usdRiPxr/resources/plugInfo.json:plugInfo.json
        ${RENDERMAN_LOCATION}/lib/usd/schemas/usdRiPxr/resources/generatedSchema.usda:generatedSchema.usda
        ${RENDERMAN_LOCATION}/lib/usd/schemas/usdRiPxr/resources/usdRiPxr/schema.usda:usdRiPxr/schema.usda
    )
# If missing check the legacy path: ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources
# This will be removed at a later date once we no longer support RenderMan 27 and below
elseif (EXISTS "${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources")
    pxr_library(usdRiPxr
        RESOURCE_FILES
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/plugInfo.json:plugInfo.json
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/generatedSchema.usda:generatedSchema.usda
        ${RENDERMAN_LOCATION}/lib/usd_plugins/usdRiPxr/Resources/usdRiPxr/schema.usda:usdRiPxr/schema.usda
    )
else()
    message(WARNING "No RenderMan location given. Could not install schemas.")
endif()
