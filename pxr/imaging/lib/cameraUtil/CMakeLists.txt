set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE cameraUtil)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(cameraUtil
    LIBRARIES
        tf
        gf

    PUBLIC_CLASSES
        conformWindow
        screenWindowParameters

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
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
