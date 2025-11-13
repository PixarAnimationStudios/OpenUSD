set(PXR_PREFIX "")
set(PXR_PACKAGE rmanArgsParser)

pxr_plugin(rmanArgsParser
    LIBRARIES
        arch
        gf
        tf
        vt
        ar
        sdr

    PRIVATE_CLASSES
        rmanArgsParser

    CPPFILES
        pugixml/pugixml.cpp
    
    PRIVATE_HEADERS
        api.h
        pugixml/pugiconfig.hpp
        pugixml/pugixml.hpp

    RESOURCE_FILES
        plugInfo.json
)
