set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_BUILD_USDVIEW)
    return()
endif()

pxr_python_bin(testusdview
    DEPENDENCIES
        usdviewq
)

if (NOT PXR_HEADLESS_TEST_MODE)
    pxr_install_test_dir(
        SRC testenv/testUsdviewAlive
        DEST testUsdviewAlive
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments1
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments2
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments3
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments4
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments5
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments6
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewFileArguments
        DEST testUsdviewFileArguments7
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewWrapper
        DEST testUsdviewWrapper1
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewWrapper
        DEST testUsdviewWrapper2
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewWrapper
        DEST testUsdviewWrapper3
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewWrapper
        DEST testUsdviewWrapper4
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewWrapper
        DEST testUsdviewWrapper5
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewPrimPathBar
        DEST testUsdviewPrimPathBar
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewPrimSearch
        DEST testUsdviewPrimSearch
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewPropertySearch
        DEST testUsdviewPropertySearch
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewMetadatatabSelect
        DEST testUsdviewMetadatatabSelect
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewTimeSamples
        DEST testUsdviewTimeSamples
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewNavigationKeys
        DEST testUsdviewNavigationKeys
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewReloadReopen
        DEST testUsdviewReloadReopen
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewNoPlugins
        DEST testUsdviewNoPlugins
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewInterpreterNoRender
        DEST testUsdviewInterpreterNoRender
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewLoadPlugins
        DEST testUsdviewLoadPlugins_alive
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewLoadPlugins
        DEST testUsdviewLoadPlugins_multipleContainers
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewLoadPlugins
        DEST testUsdviewLoadPlugins_missingContainer
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewLoadPlugins
        DEST testUsdviewLoadPlugins_duplicateCommand
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewLoadPlugins
        DEST testUsdviewLoadPlugins_chained
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewUpAxis
        DEST testUsdviewUpAxis_zUp
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewUpAxis
        DEST testUsdviewUpAxis_yUp
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewDeactivate
        DEST testUsdviewDeactivate
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewDefaultFontFamily
        DEST testUsdviewDefaultFontFamily
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewSkinning
        DEST testUsdviewSkinning
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewPrimTreeExpansion
        DEST testUsdviewPrimTreeExpansion
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewInfGeom
        DEST testUsdviewInfGeom
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewShaderEdits
        DEST testUsdviewShaderEdits
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewPrimvarEdits
        DEST testUsdviewPrimvarEdits
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewInstancingEdits
        DEST testUsdviewInstancingEdits
    )

    pxr_install_test_dir(
        SRC testenv/testUsdviewAnimatedBounds
        DEST testUsdviewAnimatedBounds
    )

    pxr_register_test(testUsdviewAlive                                               
        PRE_COMMAND "${PYTHON_EXECUTABLE} testUsdviewAliveSetup.py"
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testAlive.py test.usda"   
        EXPECTED_RETURN_CODE 0                                                       
    )

    pxr_register_test(testUsdviewFileArguments1
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usda" 
        EXPECTED_RETURN_CODE 0                                                       
    )

    pxr_register_test(testUsdviewFileArguments2
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usd" 
        EXPECTED_RETURN_CODE 0                                                       
    )

    pxr_register_test(testUsdviewFileArguments3
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usdc" 
        EXPECTED_RETURN_CODE 0                                                       
    )

    pxr_register_test(testUsdviewFileArguments4
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.py" 
        STDERR_REDIRECT py_test_out
        DIFF_COMPARE py_test_out 
        EXPECTED_RETURN_CODE 1                                                       
    )

    pxr_register_test(testUsdviewFileArguments5
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py invalidSyntax.usda" 
        CLEAN_OUTPUT "(?:[A-Za-z]:)?/(?!refSphere2).*/"
        STDERR_REDIRECT invalidSyntax_test_out
        DIFF_COMPARE invalidSyntax_test_out 
        EXPECTED_RETURN_CODE 1 
    )

    pxr_register_test(testUsdviewFileArguments6
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.txt" 
        STDERR_REDIRECT txt_test_out
        DIFF_COMPARE txt_test_out 
        EXPECTED_RETURN_CODE 1                                                       
    )

    pxr_register_test(testUsdviewFileArguments7
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py missing" 
        STDERR_REDIRECT missing_test_out
        DIFF_COMPARE missing_test_out 
        EXPECTED_RETURN_CODE 1                                                       
    )

    pxr_register_test(testUsdviewWrapper1
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback.py"
        EXPECTED_RETURN_CODE 2
    )

    pxr_register_test(testUsdviewWrapper2
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_1.py test.usda"
        EXPECTED_RETURN_CODE 1
    )

    pxr_register_test(testUsdviewWrapper3
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_2.py test.usda"
        EXPECTED_RETURN_CODE 1
    )

    pxr_register_test(testUsdviewWrapper4
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_3.py test.usda"
        EXPECTED_RETURN_CODE 1
    )

    pxr_register_test(testUsdviewWrapper5
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview test.usda"
        EXPECTED_RETURN_CODE 2
    )

    pxr_register_test(testUsdviewPrimPathBar
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimPathBar.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewPrimSearch
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimSearch.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewPropertySearch
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPropertySearch.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewMetadatatabSelect
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMetadatatabSelect.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewTimeSamples
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewTimeSamples.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewNavigationKeys
        RUN_SERIAL
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNavigationKeys.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewReloadReopen
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewReloadReopen.py test.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewNoPlugins
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNoPlugins.py --noplugins test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewInterpreterNoRender
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInterpreterNoRender.py --norender test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewLoadPlugins_alive
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_alive.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewLoadPlugins_multipleContainers
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_multipleContainers.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewLoadPlugins_missingContainer
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_missingContainer.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewLoadPlugins_duplicateCommand
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_duplicateCommand.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewLoadPlugins_chained
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_chained.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewUpAxis_zUp
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewUpAxis_zUp.py testZUp.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewUpAxis_yUp
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewUpAxis_yUp.py testYUp.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewDeactivate
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDeactivate.py test.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewDefaultFontFamily
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDefaultFontFamily.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewSkinning
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSkinning.py arm.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewPrimTreeExpansion
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimTreeExpansion.py test.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewInfGeom
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInfGeom.py infGeom.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewShaderEdits
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewShaderEdits.py test.usda"
        EXPECTED_RETURN_CODE 0
    )
    
    pxr_register_test(testUsdviewPrimvarEdits
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimvarEdits.py primvars.usda"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdviewInstancingEdits
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInstancingEdits.py test.usda"
        EXPECTED_RETURN_CODE 0
    )

pxr_register_test(testUsdviewAnimatedBounds
        PYTHON
	COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewAnimatedBounds.py test.usda"
        EXPECTED_RETURN_CODE 0
    )
    
endif()
