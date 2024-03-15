set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE HdUsdWriter)
set(optionalPublicClasses "")

pxr_plugin(HdUsdWriter
    DISABLE_PRECOMPILED_HEADERS

    LIBRARIES
        hd
        usdImaging
        usdLux
        usdShade
        usdSkel
        usdVol
        usdVolImaging

    PUBLIC_CLASSES
        camera
        curves
        instancer
        light
        material
        mesh
        openvdbasset
        points
        renderPass        
        renderDelegate
        rendererPlugin
        volume
        
    PUBLIC_HEADERS
        api.h        
        pointBased.h
        rprim.h
        utils.h

    PYMODULE_FILES
        fuzzytextdiff.py
        hdusdwriterdriver.py        

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_install_test_dir(
    SRC testenv/testUsdWriterBaseline
    DEST testUsdWriterBaseline
)

pxr_install_test_dir(
    SRC testenv/testUsdWriterProtoInstancing
    DEST testUsdWriterProtoInstancing
)

pxr_test_scripts(
    testenv/testUsdWriterBaseline.py
)

pxr_test_scripts(
    testenv/testUsdWriterProtoInstancing.py
)

pxr_register_test(testUsdWriterBaseline
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdWriterBaseline"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdWriterProtoInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdWriterProtoInstancing"
    EXPECTED_RETURN_CODE 0
)