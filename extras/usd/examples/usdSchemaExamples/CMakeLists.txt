set(PXR_PACKAGE usdSchemaExamples)

pxr_plugin(${PXR_PACKAGE}
    INCLUDE_SCHEMA_FILES

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

    PYMODULE_FILES
        __init__.py
)
   
