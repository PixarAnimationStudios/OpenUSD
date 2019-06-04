set(PXR_PREFIX "")
set(PXR_PACKAGE rmanOslParser)
add_definitions(-DPRMANBUILDINGAPI)

if(WIN32)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libmmd.lib /NODEFAULTLIB:libirc.lib /NODEFAULTLIB:svml_dispmd.lib /NODEFAULTLIB:libdecimal.lib" )
endif()

pxr_plugin(rmanOslParser
    LIBRARIES
        gf
        tf
        vt
        ar
        ndr
        sdr
        ${RENDERMAN_LIBRARY}        

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PRIVATE_CLASSES
        rmanOslParser

    RESOURCE_FILES
        plugInfo.json
)
