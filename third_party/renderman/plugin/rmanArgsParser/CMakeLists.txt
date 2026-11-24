set(PXR_PREFIX "")
set(PXR_PACKAGE rmanArgsParser)

set(optionalLibs "")
if(PXR_VERSION AND PXR_VERSION LESS 2508)
    list(APPEND optionalLibs ndr)
endif()

pxr_plugin(rmanArgsParser
    LIBRARIES
        arch
        gf
        tf
        vt
        ar
        sdf
        sdr
        ${optionalLibs}

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

if(NOT TARGET ${PXR_PACKAGE})
    return()
endif()

target_include_directories(${PXR_PACKAGE}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/third_party/renderman/plugin"
)
