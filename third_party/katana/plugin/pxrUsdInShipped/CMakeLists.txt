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

    PRIVATE_HEADERS
        declareCoreOps.h

    CPPFILES
        register.cpp

        basisCurves.cpp
        camera.cpp
        constraints.cpp
        geomSubset.cpp
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

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE

    FNASSET_STATIC
    FNATTRIBUTE_STATIC
    FNATTRIBUTEFUNCTION_STATIC
    FNATTRIBUTEMODIFIER_STATIC
    FNCONFIG_STATIC
    FNDEFAULTATTRIBUTEPRODUCER_STATIC
    FNGEOLIB_STATIC
    FNGEOLIBSERVICES_STATIC
    FNLOGGING_STATIC
    FNPLUGINMANAGER_STATIC
    FNRENDEROUTPUTLOCATION_STATIC
    FNSCENEGRAPHGENERATOR_STATIC
    FNSCENEGRAPHITERATOR_STATIC
    PYSTRING_STATIC
)

pxr_test_scripts(
    testenv/Basic/testPxrUsdInShippedBasic.py
    testenv/PointInstancer/testPxrUsdInShippedPointInstancer.py
)

pxr_install_test_dir(
    SRC testenv/Basic
    DEST testPxrUsdInShippedBasic
)

pxr_install_test_dir(
    SRC testenv/PointInstancer/oss
    DEST testPxrUsdInShippedPointInstancer
)

pxr_register_test(testPxrUsdInShippedBasic
    COMMAND "${KATANA_API_BASE_DIR}/katana --script ${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdInShippedBasic"
    EXPECTED_RETURN_CODE 0
    ENV
        KATANA_RESOURCES=${CMAKE_INSTALL_PREFIX}/third_party/katana/plugin
)

pxr_register_test(testPxrUsdInShippedPointInstancer
    COMMAND "${KATANA_API_BASE_DIR}/katana --script ${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdInShippedPointInstancer"
    EXPECTED_RETURN_CODE 0
    ENV
        KATANA_RESOURCES=${CMAKE_INSTALL_PREFIX}/third_party/katana/plugin
)
