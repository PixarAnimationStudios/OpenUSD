set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdTracy)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        trace

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}
        tracy

    PUBLIC_CLASSES
        TracyClientWrapper
    
    PYMODULE_FILES
        __init__.py

    PYMODULE_CPPFILES
        module.cpp
        wrapTracyClientWrapper.cpp

    DISABLE_PRECOMPILED_HEADERS
)

