set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)
    message(STATUS "Not building usdview because PXR_ENABLE_PYTHON_SUPPORT=OFF")
    return()
elseif (NOT TARGET usdviewq)
    message(WARNING "Not building usdview because of missing dependency: usdviewq")
    return()
endif()

pxr_python_bin(usdview
    DEPENDENCIES
        usdviewq
)

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

pxr_register_test(testUsdviewAlive                                               
    REQUIRES_DISPLAY
    PRE_COMMAND "${PYTHON_EXECUTABLE} testUsdviewAliveSetup.py"
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.usda"   
    EXPECTED_RETURN_CODE 0                                                       
)

pxr_register_test(testUsdviewFileArguments1
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.usda" 
    EXPECTED_RETURN_CODE 0                                                       
)

pxr_register_test(testUsdviewFileArguments2
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.usd" 
    EXPECTED_RETURN_CODE 0                                                       
)

pxr_register_test(testUsdviewFileArguments3
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.usdc" 
    EXPECTED_RETURN_CODE 0                                                       
)

pxr_register_test(testUsdviewFileArguments4
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.py" 
    STDERR_REDIRECT py_test_out
    DIFF_COMPARE py_test_out 
    EXPECTED_RETURN_CODE 1                                                       
)

pxr_register_test(testUsdviewFileArguments5
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup invalidSyntax.usda" 
    CLEAN_OUTPUT "\\/[t].*\\/.*\\/"
    STDERR_REDIRECT invalidSyntax_test_out
    DIFF_COMPARE invalidSyntax_test_out 
    EXPECTED_RETURN_CODE 1 
)

pxr_register_test(testUsdviewFileArguments6
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.txt" 
    STDERR_REDIRECT txt_test_out
    DIFF_COMPARE txt_test_out 
    EXPECTED_RETURN_CODE 1                                                       
)

pxr_register_test(testUsdviewFileArguments7
    REQUIRES_DISPLAY
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup missing" 
    STDERR_REDIRECT missing_test_out
    DIFF_COMPARE missing_test_out 
    EXPECTED_RETURN_CODE 1                                                       
)
