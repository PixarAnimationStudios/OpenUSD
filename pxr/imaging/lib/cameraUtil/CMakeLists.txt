set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE cameraUtil)

pxr_shared_library(cameraUtil
    LIBRARIES
        tf
        gf

    PUBLIC_CLASSES
        conformWindow
        screenWindowParameters

    CPPFILES
        moduleDeps.cpp
        
    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapConformWindow.cpp
        wrapScreenWindowParameters.cpp

    PYTHON_FILES
        __init__.py
)
