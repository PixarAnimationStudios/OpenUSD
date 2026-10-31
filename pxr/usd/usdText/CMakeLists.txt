set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdText)

set(optionalPublicClasses "")

pxr_library(usdText
    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        textLayoutAPI
        columnStyle
        columnStyleAPI
        markupText
        paragraphStyle
        paragraphStyleAPI
        tokens
        ${optionalPublicClasses}

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapColumnStyle.cpp
        wrapColumnStyleAPI.cpp
        wrapMarkupText.cpp
        wrapTextLayoutAPI.cpp
        wrapParagraphStyle.cpp
        wrapParagraphStyleAPI.cpp
        wrapTokens.cpp
        ${optionalPyModuleFiles}

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdText/schema.usda
)