set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE kind)

pxr_shared_library(kind
    LIBRARIES
        tf
        plug

    PUBLIC_CLASSES
        registry

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapRegistry.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py
)
