set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT TARGET usdviewq)
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

pxr_register_test(testUsdviewAlive                                               
    PRE_COMMAND "${PYTHON_EXECUTABLE} testUsdviewAliveSetup.py"
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdview --quitAfterStartup test.usda"   
    EXPECTED_RETURN_CODE 0                                                       
)
