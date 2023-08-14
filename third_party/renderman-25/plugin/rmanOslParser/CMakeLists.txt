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
        ${PXRCORE_LIBRARY}
        ${PRMAN_LIBRARY}        

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PRIVATE_CLASSES
        rmanOslParser

    RESOURCE_FILES
        plugInfo.json
)

# Need to set RMANTREE and RMAN_SHADERPATH environment variable when running
# tests.
#
# XXX: RMAN_SHADERPATH is only necessary for the monolithic build; in all
# other cases, the shaders accessed by the test are automatically found
# relative to libHdPrmanLoader. We may be able to remove this if we made
# it so this library wasn't embedded in the monolithic library.
get_filename_component(RMANTREE_PATH "${RENDERMAN_INCLUDE_DIR}/.." ABSOLUTE)
get_filename_component(RMANSHADER_PATH "${CMAKE_INSTALL_PREFIX}/plugin/usd/resources/shaders" ABSOLUTE)

pxr_test_scripts(
    testenv/testRmanOslParser.py
)

pxr_register_test(testRmanOslParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testRmanOslParser"
    EXPECTED_RETURN_CODE 0
    ENV
      RMANTREE=${RMANTREE_PATH}
      RMAN_SHADERPATH=${RMANSHADER_PATH}
)

