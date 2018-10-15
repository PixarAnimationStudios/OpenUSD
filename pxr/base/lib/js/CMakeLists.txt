set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE js)

pxr_library(js
    LIBRARIES
        tf

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        json
        utils
        value

    PUBLIC_HEADERS
        api.h
        converter.h
        types.h
)

pxr_build_test(testJsIO
    LIBRARIES
        tf
        js
    CPPFILES
        testenv/testJsIO.cpp
) 
pxr_build_test(testJsConverter
    LIBRARIES
        tf
        js
    CPPFILES
        testenv/testJsConverter.cpp
) 
pxr_build_test(testJsUtils
    LIBRARIES
        tf
        js
    CPPFILES
        testenv/testJsUtils.cpp
) 
pxr_build_test(testJsWriter
    LIBRARIES
        tf
        js
    CPPFILES
        testenv/testJsWriter.cpp
)
pxr_build_test(testJsDouble
    LIBRARIES
        tf
        js
    CPPFILES
        testenv/testJsDouble.cpp
)  
pxr_install_test_dir(
    SRC testenv/testJsIO
    DEST testJsIO
)
pxr_install_test_dir(
    SRC testenv/testJsIO_Errors
    DEST testJsIO_Errors
)
pxr_install_test_dir(
    SRC testenv/testJsConverter
    DEST testJsConverter
)
pxr_install_test_dir(
    SRC testenv/testJsWriter
    DEST testJsWriter
)

pxr_register_test(testJsIO_PlugInfo
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsIO plugInfo.json.in plugInfo.json"
    TESTENV
        testJsIO
    DIFF_COMPARE 
        plugInfo.json
)
pxr_register_test(testJsIO_Values
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsIO values.json.in values.json"
    TESTENV
        testJsIO
    DIFF_COMPARE 
        values.json
)
pxr_register_test(testJsIO_Errors_BadCommaPlacement
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsIO bad-commaPlacement.json -"
    TESTENV
        testJsIO_Errors
    EXPECTED_RETURN_CODE 
        2
    STDERR_REDIRECT
        testJsIO_Errors-Run1-stderr.txt
    DIFF_COMPARE 
        testJsIO_Errors-Run1-stderr.txt
)
pxr_register_test(testJsIO_Errors_MismatchedBraces
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsIO bad-mismatchedBraces.json -"
    TESTENV
        testJsIO_Errors
    EXPECTED_RETURN_CODE 
        2
    STDERR_REDIRECT
        testJsIO_Errors-Run2-stderr.txt
    DIFF_COMPARE 
        testJsIO_Errors-Run2-stderr.txt
)
pxr_register_test(testJsConverter
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsConverter"
)
pxr_register_test(testJsUtils
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsUtils"
)
pxr_register_test(testJsWriter
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsWriter test.json compact"
    TESTENV
        testJsWriter
    DIFF_COMPARE 
        test.json
)
pxr_register_test(testJsWriter_Pretty
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsWriter test-pretty.json pretty"
    TESTENV
        testJsWriter
    DIFF_COMPARE 
        test-pretty.json
)
pxr_register_test(testJsDouble
    COMMAND 
        "${CMAKE_INSTALL_PREFIX}/tests/testJsDouble"
)

