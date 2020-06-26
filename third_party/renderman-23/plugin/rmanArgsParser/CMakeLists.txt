set(PXR_PREFIX "")
set(PXR_PACKAGE rmanArgsParser)

pxr_plugin(rmanArgsParser
    LIBRARIES
        arch
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

    PYMODULE_CPPFILES
        module.cpp
        wrapRmanArgsParser.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
