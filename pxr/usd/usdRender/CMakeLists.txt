set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRender)

pxr_library(usdRender
    LIBRARIES
        gf
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        product
        settings
        settingsAPI
        settingsBase
        spec
        tokens
        var

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapProduct.cpp
        wrapSettingsAPI.cpp
        wrapSettingsBase.cpp
        wrapSettings.cpp
        wrapTokens.cpp
        wrapVar.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdRender/schema.usda

    DOXYGEN_FILES
        overview.dox
)
