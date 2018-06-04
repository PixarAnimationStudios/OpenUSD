set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdedit
    DEPENDENCIES
        sdf
        usd
)

# Look up programs needed for testing
find_program(SEDBIN
    NAMES sed 
)

find_program(CHMODBIN
    NAMES chmod
)

find_program(HEADBIN
    NAMES head
)

if (NOT EXISTS ${SEDBIN})
    message(STATUS "Skipping usdedit tests, could not find sed command.")
elseif(NOT EXISTS ${CHMODBIN})
    message(STATUS "Skipping usdedit tests, could not find chmod command.")
elseif(NOT EXISTS ${HEADBIN})
    message(STATUS "Skipping usdedit tests, could not find head command.")
else()
    pxr_install_test_dir(
        SRC testenv/testUsdEditFilePermissions
        DEST testUsdEditFilePermissions1
    )

    pxr_register_test(testUsdEditFilePermissions1
        PYTHON
        PRE_COMMAND "${CHMODBIN} -w write_protected.usda"
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdedit write_protected.usda"
        EXPECTED_RETURN_CODE 1 
        ENV
          USD_EDITOR="${HEADBIN}"
        PRE_PATH
            ${CMAKE_INSTALL_PREFIX}/bin
    )

    pxr_install_test_dir(
        SRC testenv/testUsdEditFilePermissions
        DEST testUsdEditFilePermissions2
    )

    pxr_register_test(testUsdEditFilePermissions2
        PYTHON
        PRE_COMMAND "${CHMODBIN} -w write_protected.usda"
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdedit -n write_protected.usda"
        EXPECTED_RETURN_CODE 0
        ENV
          USD_EDITOR="${HEADBIN}"
        PRE_PATH
            ${CMAKE_INSTALL_PREFIX}/bin
    )

    pxr_install_test_dir(
        SRC testenv/testUsdEditFilePermissions
        DEST testUsdEditFilePermissions3
    )

    pxr_register_test(testUsdEditFilePermissions3
        PYTHON
        PRE_COMMAND "${CHMODBIN} -w write_protected.usda"
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdedit -f write_protected.usda"
        EXPECTED_RETURN_CODE 0
        ENV
          USD_EDITOR="${HEADBIN}"
        PRE_PATH
            ${CMAKE_INSTALL_PREFIX}/bin
    )

    pxr_install_test_dir(
        SRC testenv/testUsdEditRespectFileFormat    
        DEST testUsdEditRespectFileFormat1
    )

    pxr_register_test(testUsdEditRespectFileFormat1
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdedit hello_ascii.usd"
        POST_COMMAND "${HEADBIN} -c9 hello_ascii.usd"
        POST_COMMAND_STDOUT_REDIRECT ascii_output.txt
        EXPECTED_RETURN_CODE 0
        DIFF_COMPARE ascii_output.txt
        ENV
          USD_EDITOR="sed -i 's/hello/goodbye/g'"
        PRE_PATH
            ${CMAKE_INSTALL_PREFIX}/bin
    )

    pxr_install_test_dir(
        SRC testenv/testUsdEditRespectFileFormat    
        DEST testUsdEditRespectFileFormat2
    )

    pxr_register_test(testUsdEditRespectFileFormat2
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdedit hello_crate.usd"
        POST_COMMAND "${HEADBIN} -c8 hello_crate.usd"
        POST_COMMAND_STDOUT_REDIRECT crate_output.txt   
        EXPECTED_RETURN_CODE 0
        DIFF_COMPARE crate_output.txt
        ENV
          USD_EDITOR="sed -i 's/hello/goodbye/g'"
        PRE_PATH
            ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()
