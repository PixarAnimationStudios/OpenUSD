set(PXR_PACKAGE usdSchemaExamples)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        sdf
        usd
        vt

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}

    PUBLIC_CLASSES
        simple
        complex
        paramsAPI
        tokens

    RESOURCE_FILES
        generatedSchema.usda
        plugInfo.json
)
   
