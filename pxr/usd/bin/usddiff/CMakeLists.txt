set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usddiff
    DEPENDENCIES
        tf
        sdf
)

pxr_install_test_dir(
    SRC testenv/testExitCodesForDiff
    DEST testUsdDiffExitCodesForDiff1
)

pxr_register_test(testUsdDiffExitCodesForDiff1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda a.usda --noeffect"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testExitCodesForDiff
    DEST testUsdDiffExitCodesForDiff2
)

pxr_register_test(testUsdDiffExitCodesForDiff2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda b.usda --noeffect"
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolSelectionDefault
    DEST testUsdDiffToolSelectionDefault
)

pxr_register_test(testUsdDiffToolSelectionDefault
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda b.usda --noeffect"
    STDOUT_REDIRECT out.txt
    DIFF_COMPARE out.txt
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolSelectionSingleEnvVar
    DEST testUsdDiffToolSelectionSingleEnvVar
)

pxr_register_test(testUsdDiffToolSelectionSingleEnvVar
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda b.usda --noeffect"
    STDOUT_REDIRECT out.txt
    DIFF_COMPARE out.txt
    EXPECTED_RETURN_CODE 1
    ENV
        DIFF=sdiff
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolSelectionMultiEnvVar
    DEST testUsdDiffToolSelectionMultiEnvVar
)

pxr_register_test(testUsdDiffToolSelectionMultiEnvVar
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda b.usda --noeffect"
    STDOUT_REDIRECT out.txt
    DIFF_COMPARE out.txt
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolVariousUsdFormats
    DEST testUsdDiffToolVariousFormats1
)
 
pxr_register_test(testUsdDiffToolVariousFormats1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usda a.usd --noeffect"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolVariousUsdFormats
    DEST testUsdDiffToolVariousFormats2
)

pxr_register_test(testUsdDiffToolVariousFormats2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usdc a.usd --noeffect"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolVariousUsdFormats
    DEST testUsdDiffToolVariousFormats3
)

if (PXR_BUILD_ALEMBIC_PLUGIN)
	pxr_register_test(testUsdDiffToolVariousFormats3
	    PYTHON
	    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.abc a.abc --noeffect"
	    EXPECTED_RETURN_CODE 0
	    ENV
		PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
	)

	pxr_install_test_dir(
	    SRC testenv/testDiffToolVariousUsdFormats
	    DEST testUsdDiffToolVariousFormats4
	)

	pxr_register_test(testUsdDiffToolVariousFormats4
	    PYTHON
	    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.abc a.txt --noeffect"
	    EXPECTED_RETURN_CODE 2 
	    ENV
		PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
	)

	pxr_install_test_dir(
	    SRC testenv/testDiffToolVariousUsdFormats
	    DEST testUsdDiffToolVariousFormats5
	)

	pxr_register_test(testUsdDiffToolVariousFormats5
	    PYTHON
	    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.txt a.abc --noeffect"
	    EXPECTED_RETURN_CODE 2 
	    ENV
		PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
        )
else()
    message(STATUS "Skipping alembic-based usddiff tests because PXR_BUILD_ALEMBIC_PLUGIN=OFF")
endif()

pxr_install_test_dir(
    SRC testenv/testDiffToolComposedResults
    DEST testUsdDiffComposedResults1
)

pxr_register_test(testUsdDiffComposedResults1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usd b.usd --noeffect"
    EXPECTED_RETURN_CODE 1 
    STDOUT_REDIRECT layer_diff.txt
    DIFF_COMPARE layer_diff.txt
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffToolComposedResults
    DEST testUsdDiffComposedResults2
)

pxr_register_test(testUsdDiffComposedResults2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff a.usd b.usd --noeffect --compose"
    EXPECTED_RETURN_CODE 1 
    STDOUT_REDIRECT composed_diff.txt
    DIFF_COMPARE composed_diff.txt
    CLEAN_OUTPUT "\\/tmp.*\\/"
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories1
)

pxr_register_test(testUsdDiffWithDirectories1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect a/ a/"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)
 
pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories2
)

pxr_register_test(testUsdDiffWithDirectories2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect a/ b/"
    EXPECTED_RETURN_CODE 1 
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories3
)

pxr_register_test(testUsdDiffWithDirectories3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect . a/"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories4
)

pxr_register_test(testUsdDiffWithDirectories4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect a.usda a/"
    EXPECTED_RETURN_CODE 0
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories5
)

pxr_register_test(testUsdDiffWithDirectories5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect a.usda b/"
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories6
)

pxr_register_test(testUsdDiffWithDirectories6
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect b/ a.usda"
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories7
)

pxr_register_test(testUsdDiffWithDirectories7
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect a.usda b.usda b/"
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)

pxr_install_test_dir(
    SRC testenv/testDiffWithDirectories
    DEST testUsdDiffWithDirectories8
)

pxr_register_test(testUsdDiffWithDirectories8
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect b/ a.usda b.usda"
    EXPECTED_RETURN_CODE 1
    ENV
        PATH=${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH}
)
