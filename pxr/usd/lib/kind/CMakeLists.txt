set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE kind)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(kind
    LIBRARIES
        tf
        plug

    PUBLIC_CLASSES
        registry
        
    PUBLIC_HEADERS
        api.h

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapRegistry.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py
)
