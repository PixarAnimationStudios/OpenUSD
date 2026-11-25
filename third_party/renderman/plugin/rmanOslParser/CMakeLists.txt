set(PXR_PREFIX "")
set(PXR_PACKAGE rmanOslParser)
add_definitions(-DPRMANBUILDINGAPI)

if(WIN32)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libmmd.lib /NODEFAULTLIB:libirc.lib /NODEFAULTLIB:svml_dispmd.lib /NODEFAULTLIB:libdecimal.lib /NODEFAULTLIB:libmmds.lib /NODEFAULTLIB:svml_dispmt.lib /NODEFAULTLIB:libircmt.lib" )
endif()

set(optionalLibs "")
if(PXR_VERSION AND PXR_VERSION LESS 2508)
    list(APPEND optionalLibs ndr)
endif()

if(MSVC)
    # LOADPRMAN_LIBRARY does not work on Windows debug builds because the libarary needs to also be built with debug which RenderMan does not ship.
    # If you are running a Windows debug build with rmanOslParser then you will need to add $RMANTREE/lib and $RMANTREE/bin to the path.
    list(APPEND optionalLibs
        "$<$<CONFIG:DEBUG>:${PRMAN_LIBRARY}>"
        "$<$<NOT:$<CONFIG:DEBUG>>:${LOADPRMAN_LIBRARY}>")
else()
    list(APPEND optionalLibs ${LOADPRMAN_LIBRARY})
endif()

pxr_plugin(rmanOslParser
    LIBRARIES
        arch
        gf
        tf
        vt
        ar
        sdf
        sdr
        plug
        TBB::tbb
        ${optionalLibs}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PRIVATE_CLASSES
        rmanOslParser

    RESOURCE_FILES
        plugInfo.json
)

if (NOT TARGET ${PXR_PACKAGE})
    return()
endif()

target_include_directories(${PXR_PACKAGE}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/third_party/renderman/plugin"
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

if(TARGET rmanOslParser)
    if(WIN32)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE _WIN32)
    elseif(APPLE)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE OSX)
    elseif(UNIX)
        target_compile_definitions(${PXR_PACKAGE} PRIVATE LINUX)
    endif()
endif()

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

