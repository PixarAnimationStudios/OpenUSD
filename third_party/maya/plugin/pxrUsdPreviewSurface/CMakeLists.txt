set(PXR_PACKAGE pxrUsdPreviewSurface)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        arch
        gf
        plug
        sdf
        tf
        vt
        usdMaya
        usdShade
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    PRIVATE_CLASSES
        usdPreviewSurface
        usdPreviewSurfaceShadingNodeOverride
        usdPreviewSurfaceWriter

    CPPFILES
        plugin.cpp

    PRIVATE_HEADERS
        api.h

    RESOURCE_FILES
        AEpxrUsdPreviewSurfaceTemplate.mel
        float4ToFloatW.xml
        float4ToFloatX.xml
        float4ToFloatY.xml
        float4ToFloatZ.xml
        lightingContributions.xml
        plugInfo.json
        usdPreviewSurface.xml
        usdPreviewSurfaceCombiner.xml
        usdPreviewSurfaceLighting.xml

    DISABLE_PRECOMPILED_HEADERS
)

# pxrUsdPreviewSurfaceGenFragments --validate ensures that the generated Maya
# fragment XML source files are up to date with the glslfx file and the
# contents of the templates used to generate them. If not, a diff displaying
# the difference will be printed to stdout and the build will fail.
# Note that the XSD file for Viewport 2.0 fragments seems to be misnamed in the
# Maya installation ("v2p..." as opposed to "vp2...").
if (NOT ${PXR_VALIDATE_GENERATED_CODE})
    message(STATUS "Skipping validation of Maya fragment XML files because PXR_VALIDATE_GENERATED_CODE=OFF")
else()
    add_custom_command(
        TARGET pxrUsdPreviewSurface
        POST_BUILD
        COMMENT "Validating generated Maya fragments for ${PXR_PACKAGE}"
        COMMAND
            ${PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/pxrUsdPreviewSurfaceGenFragments.py
                ${CMAKE_SOURCE_DIR}/pxr/usdImaging/lib/usdShaders/shaders/previewSurface.glslfx
                --srcDir ${CMAKE_CURRENT_SOURCE_DIR}
                --schema ${MAYA_BASE_DIR}/bin/ShadeFragment/v2pFragment.xsd
                --validate
    )
endif()

pxr_test_scripts(
    testenv/testPxrUsdPreviewSurfaceExport.py
)

pxr_install_test_dir(
    SRC testenv/PxrUsdPreviewSurfaceExportTest
    DEST testPxrUsdPreviewSurfaceExport
)
pxr_register_test(testPxrUsdPreviewSurfaceExport
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdPreviewSurfaceExport"
    TESTENV testPxrUsdPreviewSurfaceExport
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugins/pxrUsdPreviewSurface/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
