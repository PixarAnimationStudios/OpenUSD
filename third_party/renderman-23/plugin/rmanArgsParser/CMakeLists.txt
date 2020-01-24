set(PXR_PREFIX "")
set(PXR_PACKAGE rmanArgsParser)

pxr_plugin(rmanArgsParser
    LIBRARIES
        gf
        tf
        vt
        ar
        ndr
        sdr

    PRIVATE_CLASSES
        rmanArgsParser

    CPPFILES
        pugixml/pugixml.cpp
    
    PRIVATE_HEADERS
        api.h
        pugixml/pugiconfig.hpp
        pugixml/pugixml.hpp

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapRmanArgsParser.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
