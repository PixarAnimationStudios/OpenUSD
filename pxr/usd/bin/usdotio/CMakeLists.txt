set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

set(_usdOtioDirectory resources)

install(
    FILES
    ${_usdOtioDirectory}/generatedSchema.usda
    ${_usdOtioDirectory}/plugInfo.json
    ${_usdOtioDirectory}/schema.usda
    
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdotio
)


pxr_python_bin(usdotio
    DEPENDENCIES
    tf
    sdf
)

install(
    DIRECTORY lib/python/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/usdotio
    )

pxr_install_test_dir(
    SRC testenv/testUsdOtioAdd1
    DEST testUsdOtioAdd1
)

pxr_register_test(testUsdOtioAdd1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio add -o out.usda -y -n a.otio a.usda"
    DIFF_COMPARE out.usda
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioAdd2
    DEST testUsdOtioAdd2
)

pxr_register_test(testUsdOtioAdd2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio add -o out.usda -y -n a.otio a.usda"
    DIFF_COMPARE out.usda
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioAdd3
    DEST testUsdOtioAdd3
)

pxr_register_test(testUsdOtioAdd3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio add -o out.usda -y -n a.otio a.usda"
    DIFF_COMPARE out.usda
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioAdd4
    DEST testUsdOtioAdd4
)

pxr_register_test(testUsdOtioAdd4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio add -o out.usda -y -n a.otio a.usda"
    DIFF_COMPARE out.usda
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioAdd5
    DEST testUsdOtioAdd5
)

pxr_register_test(testUsdOtioAdd5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio add -o out.usda -y -n a.otio a.usda"
    DIFF_COMPARE out.usda
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioSave1
    DEST testUsdOtioSave1
)

pxr_register_test(testUsdOtioSave1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio save -y out.otio a.usda"
    DIFF_COMPARE out.otio
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioSave2
    DEST testUsdOtioSave2
)

pxr_register_test(testUsdOtioSave2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio save -y out.otio a.usda"
    DIFF_COMPARE out.otio
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioSave3
    DEST testUsdOtioSave3
)

pxr_register_test(testUsdOtioSave3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio save -y out.otio a.usda"
    DIFF_COMPARE out.otio
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioSave4
    DEST testUsdOtioSave4
)

pxr_register_test(testUsdOtioSave4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio save -y out.otio a.usda"
    DIFF_COMPARE out.otio
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)

pxr_install_test_dir(
    SRC testenv/testUsdOtioSave5
    DEST testUsdOtioSave5
)

pxr_register_test(testUsdOtioSave5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdotio save -y out.otio a.usda"
    DIFF_COMPARE out.otio
    EXPECTED_RETURN_CODE 0
    PRE_PATH ${CMAKE_INSTALL_PREFIX}/bin
)
