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

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        simple
        complex
        paramsAPI
        tokens

    CPPFILES
        moduleDeps.cpp    

    PYTHON_FILES
        __init__.py

    PYMODULE_CPPFILES
        module.cpp
        wrapComplex.cpp
        wrapParamsAPI.cpp
        wrapSimple.cpp
        wrapTokens.cpp        

    RESOURCE_FILES
        generatedSchema.usda
        plugInfo.json
)
   
