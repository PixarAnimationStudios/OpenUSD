set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdstitchclips
    DEPENDENCIES
        tf
        sdf
        usdUtils
)

pxr_install_test_dir(
    SRC testenv/testInvalidClipPaths
    DEST testUsdStitchClipsInvalidClipPaths1
)

pxr_register_test(testUsdStitchClipsInvalidClipPaths1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips a.usd b.usd --clipPath /World/fx/Particles_Splash --out test1.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testInvalidClipPaths
    DEST testUsdStitchClipsInvalidClipPaths2
)

pxr_register_test(testUsdStitchClipsInvalidClipPaths2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips a.usd b.usd --clipPath /World/f_x --out test2.usd"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testFileCleanup
    DEST testUsdStitchClipsFileCleanup1
)

pxr_register_test(testUsdStitchClipsFileCleanup1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --clipPath /Wrong/Clip/Path --out result.usd a.usd b.usd"
    EXPECTED_RETURN_CODE 1
    FILES_DONT_EXIST result.usd result.topology.usd
)

pxr_install_test_dir(
    SRC testenv/testFileCleanup
    DEST testUsdStitchClipsFileCleanup2
)

pxr_register_test(testUsdStitchClipsFileCleanup2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --clipPath /Wrong/Clip/Path --out preexisting.usd a.usd b.usd"
    EXPECTED_RETURN_CODE 1
    FILES_EXIST preexisting.topology.usd
    FILES_DONT_EXIST preexisting.usd
)

pxr_install_test_dir(
    SRC testenv/testFileCleanup
    DEST testUsdStitchClipsFileCleanup3
)

pxr_register_test(testUsdStitchClipsFileCleanup3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --clipPath /Wrong/Clip/Path --out nonexistant.usd a.usd b.usd"
    EXPECTED_RETURN_CODE 1
    FILES_DONT_EXIST nonexistant.usd nonexistant.topology.usd 
)

pxr_install_test_dir(
    SRC testenv/testStitchClips
    DEST testUsdStitchClips
)

pxr_register_test(testUsdStitchClips
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --clipPath
    /World/fx/Particles_Splash --out result.usda Particles_Splash.101.usd Particles_Splash.102.usd Particles_Splash.103.usd Particles_Splash.104.usd Particles_Splash.105.usd Particles_Splash.106.usd Particles_Splash.107.usd Particles_Splash.108.usd Particles_Splash.109.usd "
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE result.usda result.topology.usda
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate1
)

pxr_register_test(testUsdStitchClipsTemplate1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --startTimeCode 101 --endTimeCode 104 --stride 1 --templatePath p.#.usd --clipPath /World/fx/Particles_Splash/points --out result.usda p.101.usda p.102.usda p.103.usda p.104.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE result.usda result.topology.usda
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate2
)

pxr_register_test(testUsdStitchClipsTemplate2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --endTimeCode 104 --stride 1 --templatePath p.#.usd --clipPath /World/fx/Particles_Splash p.*.usda"
    EXPECTED_RETURN_CODE 1 
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate3
)

pxr_register_test(testUsdStitchClipsTemplate3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --startTimeCode 101 --stride 1 --templatePath p.#.usd --clipPath /World/fx/Particles_Splash p.*.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate4
)

pxr_register_test(testUsdStitchClipsTemplate4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --startTimeCode 101 --endTimeCode 104 --templatePath p.#.usd --clipPath /World/fx/Particles_Splash p.*.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate5
)

pxr_register_test(testUsdStitchClipsTemplate5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --startTimeCode 101 --endTimeCode 104 --stride 1 --clipPath /World/fx/Particles_Splash p.*.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplate6
)

pxr_register_test(testUsdStitchClipsTemplate6
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --templateMetadata --startTimeCode 101 --endTimeCode 104 --stride 1 --templatePath p.#.usd p.*.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testStitchClips
    DEST testUsdStitchClipsCustomSetName
)

pxr_register_test(testUsdStitchClipsCustomSetName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --clipSet bob --clipPath /World/fx/Particles_Splash --out result_customSetName.usda Particles_Splash.101.usd Particles_Splash.102.usd Particles_Splash.103.usd Particles_Splash.104.usd Particles_Splash.105.usd Particles_Splash.106.usd Particles_Splash.107.usd Particles_Splash.108.usd Particles_Splash.109.usd "
    EXPECTED_RETURN_CODE 0 
    DIFF_COMPARE result_customSetName.usda result_customSetName.topology.usda
)

pxr_install_test_dir(
    SRC testenv/testStitchClipsTemplate
    DEST testUsdStitchClipsTemplateCustomSetName
)

pxr_register_test(testUsdStitchClipsTemplateCustomSetName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdstitchclips --noComment --clipSet bob --templateMetadata --startTimeCode 101 --endTimeCode 104 --stride 1 --templatePath p.#.usd --clipPath /World/fx/Particles_Splash/points --out result_customSetName.usda p.101.usda p.102.usda p.103.usda p.104.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE result_customSetName.usda result_customSetName.topology.usda
)

