set(PXR_PACKAGE pxrUsdInShipped)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        katanaAttrfncApi
        katanaOpApi
        katanaPluginApi
        kind
        usd
        usdGeom
        usdKatana
        usdShade
        usdLux
        usdRi

    PRIVATE_CLASSES
        attrfnc_materialReference
        pointInstancerUtils

    PRIVATE_HEADERS
        declareCoreOps.h

    CPPFILES
        register.cpp

        basisCurves.cpp
        camera.cpp
        constraints.cpp
        light.cpp
        lightFilter.cpp
        material.cpp
        materialsGroup.cpp
        mesh.cpp
        model.cpp
        nurbsPatch.cpp
        pointInstancer.cpp
        points.cpp
        scope.cpp
        uiUtils.cpp
        xform.cpp
)

pxr_install_test_dir(
    SRC testenv/Basic
    DEST testPxrUsdInShippedBasic
)

pxr_test_scripts(
    testenv/Basic/testPxrUsdInShippedBasic.py
)

pxr_register_test(testPxrUsdInShippedBasic
    COMMAND "${KATANA_API_BASE_DIR}/katana --script ${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdInShippedBasic"
    EXPECTED_RETURN_CODE 0
    ENV
        KATANA_RESOURCES=${CMAKE_INSTALL_PREFIX}/third_party/katana/plugin
)
