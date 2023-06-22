set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingDX)


pxr_library(usdImagingDX
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        hio
        garch
        glf
        hd
        hdx
        pxOsd
        sdf
        sdr
        usd
        usdGeom
        usdHydra
        usdShade
        usdImaging

    INCLUDE_DIRS

    PUBLIC_CLASSES
        dummy

    PUBLIC_HEADERS

    PRIVATE_HEADERS
        dummy.h
        unitTestDXDrawing.h
#        testEnv/shadersDefines.h

    RESOURCE_FILES
)


#
# C++ tests - Only build on WINDOWS platform.
# TODO
# 
set(TEST_LIBS
        "arch"
        "garch"
        "glf"
        "hd"
        "hgi"
        "hgiDX"
        "sdf"
        "tf"
        "usd"
        "usdUtils"
        "usdImaging"
        "usdImagingGL")

pxr_build_test(testUsdImagingDXBasicDrawing
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestDXDrawing.cpp
        testenv/testUsdImagingDXBasicDrawing.cpp
)

#
# Install all the test data
#

pxr_install_test_dir(
    SRC testenv/testUsdImagingDXBasicDrawing
    DEST testUsdImagingDXBasicDrawing
)

#
# Register all the tests
#

# very simple sphere - expect complexity 1.0: pretty rough contour
pxr_register_test(testUsdImagingDXBasicDrawing_sph_10
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -frameAll -stage basicDrawing/sphere.usda -write sphere_1.0.png"
    IMAGE_DIFF_COMPARE
        sphere_1.0.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# very simple sphere - complexity 1.1: better looking
pxr_register_test(testUsdImagingDXBasicDrawing_sph_11
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -complexities 1.1 -offscreen -frameAll -stage basicDrawing/sphere.usda -write sphere.png"
    IMAGE_DIFF_COMPARE
        sphere_1.1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# force software rendering
# very simple sphere - complexity 1.1: better looking
pxr_register_test(testUsdImagingDXBasicDrawing_warp_sph_11
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -complexities 1.1 -offscreen -frameAll -stage basicDrawing/sphere.usda -write sphere.png"
    IMAGE_DIFF_COMPARE
        sphere_1.1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HGI_DX_FORCE_WARP=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -frameAll -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto)
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto) and some lighting
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# force software rendering
# a scene with all out of the box primitives and a better placed camera (auto) and some lighting
pxr_register_test(testUsdImagingDXBasicDrawing_warp_allPrims_3d_cam_lights
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HGI_DX_FORCE_WARP=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto) and some lighting - points
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights_pts
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -shading points -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l_pts.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l_pts.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto) and some lighting - wireframe
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -shading wire -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l_w.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l_w.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto) and some lighting - wireframe on surface
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam_wos
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -shading wireOnSurface -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l_wos.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l_wos.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# force software rendering
# a scene with all out of the box primitives and a better placed camera (auto) and some lighting - wireframe on surface
pxr_register_test(testUsdImagingDXBasicDrawing_warp_allPrims_3d_cam_wos
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -shading wireOnSurface -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l_wos.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l_wos.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HGI_DX_FORCE_WARP=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# a scene with all out of the box primitives and a better placed camera (auto) and some lighting - flat
pxr_register_test(testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights_flat
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDXBasicDrawing -offscreen -autoCamPosAndDir -lighting -shading flat -stage basicDrawing/basicOOBPrims.usda -write basicOOBPrims_1.0_3d_l_flat.png"
    IMAGE_DIFF_COMPARE
        basicOOBPrims_1.0_3d_l_flat.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingDXBasicDrawing
    ENV
        HGI_ENABLE_DX=1
        HDST_ENABLE_HGI_RESOURCE_GENERATION=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

