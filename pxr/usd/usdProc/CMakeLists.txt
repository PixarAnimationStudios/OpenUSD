set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdProc)

pxr_library(usdProc
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox

)
