set(PXR_PACKAGE usdMaya)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        ar
        gf
        js
        kind
        plug
        sdf
        tf
        usd
        usdGeom
        usdLux
        usdRi
        usdShade
        usdSkel
        usdUtils
        vt
        ${Boost_PYTHON_LIBRARY}
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaAnim_LIBRARY}
        ${MAYA_OpenMayaFX_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        adaptor
        colorSpace
        editUtil
        stageCache
        stageData
        meshUtil
        primReaderArgs
        primReaderContext
        primReaderRegistry
        primWriterArgs
        primWriterContext
        primWriterRegistry
        query
        translatorCamera
        translatorCurves
        translatorGprim
        translatorMaterial
        translatorMesh
        translatorModelAssembly
        translatorNurbsPatch
        translatorPrim
        translatorRfMLight
        translatorSkel
        translatorUtil
        translatorXformable
        readUtil
        roundTripUtil
        shadingModeExporter
        shadingModeExporterContext
        shadingModeImporter
        shadingModeRegistry
        skelBindingsWriter
        util
        usdPrimProvider
        writeUtil
        xformStack

        MayaPrimWriter
        MayaTransformWriter
        FunctorPrimWriter

        primReader
        functorPrimReader

        JobArgs
        usdReadJob
        usdWriteJob
        usdWriteJobCtx

        pluginStaticData
        proxyShape
        referenceAssembly
        usdImport
        usdExport
        usdListShadingModes
        usdTranslatorExport
        usdTranslatorImport

        Chaser
        ChaserRegistry

        UserTaggedAttribute

    PRIVATE_CLASSES
        debugCodes
        registryHelper
        MayaCameraWriter
        MayaInstancerWriter
        MayaLocatorWriter
        MayaMeshWriter
        MayaNurbsCurveWriter
        MayaNurbsSurfaceWriter
        MayaParticleWriter
        MayaSkeletonWriter
        ModelKindWriter

    PRIVATE_HEADERS
        shadingModePxrRis_rfm_map.h

    CPPFILES
        MayaMeshWriter_PrimVars.cpp
        MayaMeshWriter_Skin.cpp
        MayaMeshWriter_SubDiv.cpp
        primReaderCamera.cpp
        primReaderMaterial.cpp
        primReaderMesh.cpp
        primReaderNurbsCurves.cpp
        primReaderNurbsPatch.cpp
        primReaderSkel.cpp
        primReaderXform.cpp
        shadingModeDisplayColor.cpp
        shadingModePxrRis.cpp
        translatorMesh_PrimVars.cpp
        translatorMesh_SubDiv.cpp
        translatorXformable_decompose.cpp
        usdReadJob_ImportWithProxies.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAssembly.cpp
        wrapColorSpace.cpp
        wrapEditUtil.cpp
        wrapMeshUtil.cpp
        wrapQuery.cpp
        wrapReadUtil.cpp
        wrapRoundTripUtil.cpp
        wrapStageCache.cpp
        wrapAdaptor.cpp
        wrapUserTaggedAttribute.cpp
        wrapWriteUtil.cpp
        wrapXformStack.cpp

    PYMODULE_FILES
        __init__.py
        AEpxrUsdReferenceAssemblyTemplate.py
        userExportedAttributesUI.py

    RESOURCE_FILES
        AEpxrUsdProxyShapeTemplate.mel
        out_pxrUsdProxyShape.xpm
        out_pxrUsdReferenceAssembly.xpm
        usdMaya.mel
        usdTranslatorExport.mel
        usdTranslatorImport.mel

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
        testenv/testUsdExportAssembly.py
        testenv/testUsdExportCamera.py
        testenv/testUsdExportColorSets.py
        testenv/testUsdExportDisplayColor.py
        testenv/testUsdExportFrameOffset.py
        testenv/testUsdExportInstances.py
        testenv/testUsdExportLocator.py
        testenv/testUsdExportMesh.py
        testenv/testUsdExportNurbsCurve.py
        testenv/testUsdExportOverImport.py
        testenv/testUsdExportParentScope.py
        testenv/testUsdExportParticles.py
        testenv/testUsdExportPointInstancer.py
        testenv/testUsdExportRenderLayerMode.py
        testenv/testUsdExportRfMLight.py
        testenv/testUsdExportSelection.py
        testenv/testUsdExportShadingModeDisplayColor.py
        testenv/testUsdExportShadingModePxrRis.py
        testenv/testUsdExportSkeleton.py
        testenv/testUsdExportUVSets.py
        testenv/testUsdImportAsAssemblies.py
        testenv/testUsdImportCamera.py
        testenv/testUsdImportColorSets.py
        testenv/testUsdImportFrameRange.py
        testenv/testUsdImportNestedAssemblyAnimation.py
        testenv/testUsdImportRfMLight.py
        testenv/testUsdImportSessionLayer.py
        testenv/testUsdImportShadingModeDisplayColor.py
        testenv/testUsdImportShadingModePxrRis.py
        testenv/testUsdImportSkeleton.py
        testenv/testUsdImportUVSets.py
        testenv/testUsdImportXforms.py
        testenv/testUsdMayaAdaptor.py
        testenv/testUsdMayaAppDir.py
        testenv/testUsdMayaGetVariantSetSelections.py
        testenv/testUsdMayaModelKindWriter.py
        testenv/testUsdMayaProxyShape.py
        testenv/testUsdMayaReadWriteUtils.py
        testenv/testUsdMayaReferenceAssemblyEdits.py
        testenv/testUsdMayaUserExportedAttributes.py
        testenv/testUsdMayaXformStack.py
        testenv/testUsdGeomAttributeConverters.py
        testenv/testUsdMetadataAttributeConverters.py
        testenv/testUsdReferenceAssemblyChangeRepresentations.py
        testenv/testUsdReferenceAssemblySelection.py
)

# Note - we set up a maya profile directory ($MAYA_APP_DIR) that's empty, so we
# can ensure we test with a default profile - had some crashes with 2017 when
# using an existing user profile (due to some problems with color management
# settings)
# Also, maya creates the $MAYA_APP_DIR on demand, so we don't need to bother
# making the directory ourselves.  We use <PXR_TEST_DIR>, which is expanded
# to the absolute path of the temporary folder tests are run in, because:
#   A) we can't use relative paths ("./maya_profile"), because maya in
#      windows will error
#   B) we don't know the absolute path to the test directory at cmake-compile
#      time 

pxr_install_test_dir(
    SRC testenv/UsdExportAssemblyTest
    DEST testUsdExportAssembly
)
pxr_register_test(testUsdExportAssembly
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportAssembly"
    TESTENV testUsdExportAssembly
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportCameraTest
    DEST testUsdExportCamera
)
pxr_register_test(testUsdExportCamera
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportCamera"
    TESTENV testUsdExportCamera
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportColorSetsTest
    DEST testUsdExportColorSets
)
pxr_register_test(testUsdExportColorSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportColorSets"
    TESTENV testUsdExportColorSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportDisplayColorTest
    DEST testUsdExportDisplayColor
)
pxr_register_test(testUsdExportDisplayColor
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportDisplayColor"
    TESTENV testUsdExportDisplayColor
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportFrameOffsetTest
    DEST testUsdExportFrameOffset
)
pxr_register_test(testUsdExportFrameOffset
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportFrameOffset"
    TESTENV testUsdExportFrameOffset
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportInstancesTest
    DEST testUsdExportInstances
)
pxr_register_test(testUsdExportInstances
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportInstances"
    TESTENV testUsdExportInstances
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportLocatorTest
    DEST testUsdExportLocator
)
pxr_register_test(testUsdExportLocator
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportLocator"
    TESTENV testUsdExportLocator
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportMeshTest
    DEST testUsdExportMesh
)
pxr_register_test(testUsdExportMesh
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportMesh"
    TESTENV testUsdExportMesh
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportNurbsCurveTest
    DEST testUsdExportNurbsCurve
)
pxr_register_test(testUsdExportNurbsCurve
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportNurbsCurve"
    TESTENV testUsdExportNurbsCurve
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportOverImportTest
    DEST testUsdExportOverImport
)
pxr_register_test(testUsdExportOverImport
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportOverImport"
    TESTENV testUsdExportOverImport
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportParentScopeTest
    DEST testUsdExportParentScope
)
pxr_register_test(testUsdExportParentScope
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportParentScope"
    TESTENV testUsdExportParentScope
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportParticlesTest
    DEST testUsdExportParticles
)
pxr_register_test(testUsdExportParticles
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportParticles"
    TESTENV testUsdExportParticles
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportPointInstancerTest
    DEST testUsdExportPointInstancer
)
pxr_register_test(testUsdExportPointInstancer
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportPointInstancer"
    TESTENV testUsdExportPointInstancer
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportRenderLayerModeTest
    DEST testUsdExportRenderLayerMode
)
pxr_register_test(testUsdExportRenderLayerMode
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportRenderLayerMode"
    TESTENV testUsdExportRenderLayerMode
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
        MAYA_ENABLE_LEGACY_RENDER_LAYERS=1
)

# XXX: This test is disabled by default since it requires the RenderMan for
# Maya plugin.
# pxr_install_test_dir(
#     SRC testenv/UsdExportRfMLightTest
#     DEST testUsdExportRfMLight
# )
# pxr_register_test(testUsdExportRfMLight
#     CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
#     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportRfMLight"
#     TESTENV testUsdExportRfMLight
#     ENV
#         MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
#         MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
#         MAYA_DISABLE_CIP=1
#         MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
# )

pxr_install_test_dir(
    SRC testenv/UsdExportSelectionTest
    DEST testUsdExportSelection
)
pxr_register_test(testUsdExportSelection
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportSelection"
    TESTENV testUsdExportSelection
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportShadingModeDisplayColor
    DEST testUsdExportShadingModeDisplayColor
)
pxr_register_test(testUsdExportShadingModeDisplayColor
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportShadingModeDisplayColor"
    TESTENV testUsdExportShadingModeDisplayColor
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
        USD_RI_WRITE_BXDF_OUTPUT=0
)

pxr_install_test_dir(
    SRC testenv/UsdExportShadingModePxrRis
    DEST testUsdExportShadingModePxrRis
)
pxr_register_test(testUsdExportShadingModePxrRis
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportShadingModePxrRis"
    TESTENV testUsdExportShadingModePxrRis
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
        USD_RI_WRITE_BXDF_OUTPUT=0
)

pxr_install_test_dir(
    SRC testenv/UsdExportSkeletonTest
    DEST testUsdExportSkeleton
)
pxr_register_test(testUsdExportSkeleton
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportSkeleton"
    TESTENV testUsdExportSkeleton
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportUVSetsTest
    DEST testUsdExportUVSets
)
pxr_register_test(testUsdExportUVSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportUVSets"
    TESTENV testUsdExportUVSets
    ENV
        PIXMAYA_WRITE_UV_AS_FLOAT2=0
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportFloatUVSetsTest
    DEST testUsdExportFloatUVSets
)
pxr_register_test(testUsdExportFloatUVSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExportUVSets"
    TESTENV testUsdExportFloatUVSets
    ENV
        PIXMAYA_WRITE_UV_AS_FLOAT2=1
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportAsAssemblies
    DEST testUsdImportAsAssemblies
)
pxr_register_test(testUsdImportAsAssemblies
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportAsAssemblies"
    TESTENV testUsdImportAsAssemblies
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportCameraTest
    DEST testUsdImportCamera
)
pxr_register_test(testUsdImportCamera
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportCamera"
    TESTENV testUsdImportCamera
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportColorSetsTest
    DEST testUsdImportColorSets
)
pxr_register_test(testUsdImportColorSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportColorSets"
    TESTENV testUsdImportColorSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportFrameRangeTest
    DEST testUsdImportFrameRange
)
pxr_register_test(testUsdImportFrameRange
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportFrameRange"
    TESTENV testUsdImportFrameRange
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportNestedAssemblyAnimationTest
    DEST testUsdImportNestedAssemblyAnimation
)
pxr_register_test(testUsdImportNestedAssemblyAnimation
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportNestedAssemblyAnimation"
    TESTENV testUsdImportNestedAssemblyAnimation
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

# XXX: This test is disabled by default since it requires the RenderMan for
# Maya plugin.
# pxr_install_test_dir(
#     SRC testenv/UsdImportRfMLightTest
#     DEST testUsdImportRfMLight
# )
# pxr_register_test(testUsdImportRfMLight
#     CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
#     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportRfMLight"
#     TESTENV testUsdImportRfMLight
#     ENV
#         MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
#         MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
#         MAYA_DISABLE_CIP=1
#         MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
# )

pxr_install_test_dir(
    SRC testenv/UsdImportSessionLayerTest
    DEST testUsdImportSessionLayer
)
pxr_register_test(testUsdImportSessionLayer
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportSessionLayer"
    TESTENV testUsdImportSessionLayer
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportShadingModeDisplayColor
    DEST testUsdImportShadingModeDisplayColor
)
pxr_register_test(testUsdImportShadingModeDisplayColor
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportShadingModeDisplayColor"
    TESTENV testUsdImportShadingModeDisplayColor
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportShadingModePxrRis
    DEST testUsdImportShadingModePxrRis
)
pxr_register_test(testUsdImportShadingModePxrRis
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportShadingModePxrRis"
    TESTENV testUsdImportShadingModePxrRis
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportSkeleton
    DEST testUsdImportSkeleton
)
pxr_register_test(testUsdImportSkeleton
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportSkeleton"
    TESTENV testUsdImportSkeleton
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportUVSetsTest
    DEST testUsdImportUVSets
)
pxr_register_test(testUsdImportUVSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportUVSets"
    TESTENV testUsdImportUVSets
    ENV
        PIXMAYA_READ_FLOAT2_AS_UV=0
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportFloatUVSetsTest
    DEST testUsdImportFloatUVSets
)
pxr_register_test(testUsdImportFloatUVSets
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportUVSets"
    TESTENV testUsdImportFloatUVSets
    ENV
        PIXMAYA_READ_FLOAT2_AS_UV=1
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportXformsTest
    DEST testUsdImportXforms
)
pxr_register_test(testUsdImportXforms
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImportXforms"
    TESTENV testUsdImportXforms
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaAppDirTest
    DEST testUsdMayaAppDir
)
pxr_register_test(testUsdMayaAppDir
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaAppDir"
    TESTENV testUsdMayaAppDir
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaGetVariantSetSelectionsTest
    DEST testUsdMayaGetVariantSetSelections
)
pxr_register_test(testUsdMayaGetVariantSetSelections
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaGetVariantSetSelections"
    TESTENV testUsdMayaGetVariantSetSelections
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaModelKindWriterTest
    DEST testUsdMayaModelKindWriter
)
pxr_register_test(testUsdMayaModelKindWriter
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaModelKindWriter"
    TESTENV testUsdMayaModelKindWriter
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaProxyShape
    DEST testUsdMayaProxyShape
)
pxr_register_test(testUsdMayaProxyShape
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaProxyShape"
    TESTENV testUsdMayaProxyShape
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaReferenceAssemblyEdits
    DEST testUsdMayaReferenceAssemblyEdits
)
pxr_register_test(testUsdMayaReferenceAssemblyEdits
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaReferenceAssemblyEdits"
    TESTENV testUsdMayaReferenceAssemblyEdits
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaUserExportedAttributesTest
    DEST testUsdMayaUserExportedAttributes
)
pxr_register_test(testUsdMayaUserExportedAttributes
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaUserExportedAttributes"
    TESTENV testUsdMayaUserExportedAttributes
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdGeomAttributeConvertersTest
    DEST testUsdGeomAttributeConverters
)
pxr_register_test(testUsdGeomAttributeConverters
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdGeomAttributeConverters"
    TESTENV testUsdGeomAttributeConverters
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMetadataAttributeConvertersTest
    DEST testUsdMetadataAttributeConverters
)
pxr_register_test(testUsdMetadataAttributeConverters
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMetadataAttributeConverters"
    TESTENV testUsdMetadataAttributeConverters
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblyChangeRepresentationsTest
    DEST testUsdReferenceAssemblyChangeRepresentations
)
pxr_register_test(testUsdReferenceAssemblyChangeRepresentations
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdReferenceAssemblyChangeRepresentations"
    TESTENV testUsdReferenceAssemblyChangeRepresentations
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblySelectionTest
    DEST testUsdReferenceAssemblySelection
)
pxr_register_test(testUsdReferenceAssemblySelection
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdReferenceAssemblySelection"
    TESTENV testUsdReferenceAssemblySelection
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_register_test(testUsdMayaXformStack
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaXformStack"
    TESTENV testUsdMayaXformStack
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
