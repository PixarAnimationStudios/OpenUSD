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
        usdRi
        usdShade
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
        translatorUtil
        translatorXformable
        roundTripUtil
        shadingModeExporter
        shadingModeExporterContext
        shadingModeImporter
        shadingModeRegistry
        util
        usdPrimProvider
        writeUtil

        MayaPrimWriter
        MayaTransformWriter
        FunctorPrimWriter
        JobArgs
        usdReadJob
        usdWriteJob

        pluginStaticData
        proxyShape
        referenceAssembly
        usdImport
        usdExport
        usdTranslator

        Chaser
        ChaserRegistry
       
        AttributeConverter
        AttributeConverterRegistry

        UserTaggedAttribute

    PRIVATE_CLASSES
        debugCodes
        registryHelper
        MayaCameraWriter
        MayaMeshWriter
        MayaNurbsCurveWriter
        MayaNurbsSurfaceWriter
        ModelKindWriter

    PRIVATE_HEADERS
        shadingModePxrRis_rfm_map.h

    CPPFILES
        MayaMeshWriter_PrimVars.cpp
        MayaMeshWriter_SubDiv.cpp
        primReaderCamera.cpp
        primReaderMaterial.cpp
        primReaderMesh.cpp
        primReaderNurbsCurves.cpp
        primReaderNurbsPatch.cpp
        primReaderXform.cpp
        shadingModeDisplayColor.cpp
        shadingModePxrRis.cpp
        translatorMesh_PrimVars.cpp
        translatorMesh_SubDiv.cpp
        translatorXformable_decompose.cpp
        usdReadJob_ImportWithProxies.cpp
        UsdMetadataAttributeConverters.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAssembly.cpp
        wrapEditUtil.cpp
        wrapQuery.cpp
        wrapRoundTripUtil.cpp
        wrapStageCache.cpp
        wrapUserTaggedAttribute.cpp

    PYTHON_FILES
        __init__.py
        AEpxrUsdReferenceAssemblyTemplate.py
        userExportedAttributesUI.py
        testenv/testUsdExportAssembly.py
        testenv/testUsdExportCamera.py
        testenv/testUsdExportColorSets.py
        testenv/testUsdExportDisplayColor.py
        testenv/testUsdExportFrameOffset.py
        testenv/testUsdExportMesh.py
        testenv/testUsdExportRenderLayerMode.py
        testenv/testUsdExportSelection.py
        testenv/testUsdExportUVSets.py
        testenv/testUsdImportAsAssemblies.py
        testenv/testUsdImportCamera.py
        testenv/testUsdImportColorSets.py
        testenv/testUsdImportFrameRange.py
        testenv/testUsdImportNestedAssemblyAnimation.py
        testenv/testUsdImportSessionLayer.py
        testenv/testUsdImportUVSets.py
        testenv/testUsdMayaGetVariantSetSelections.py
        testenv/testUsdMayaModelKindWriter.py
        testenv/testUsdMayaProxyShape.py
        testenv/testUsdMayaUserExportedAttributes.py
        testenv/testUsdMetadataAttributeConverters.py
        testenv/testUsdReferenceAssemblyChangeRepresentations.py
        testenv/testUsdReferenceAssemblySelection.py

    RESOURCE_FILES
        AEpxrUsdProxyShapeTemplate.mel
        out_pxrUsdProxyShape.xpm
        out_pxrUsdReferenceAssembly.xpm
        usdMaya.mel
        usdTranslator.mel
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
        testenv/testUsdExportMesh.py
        testenv/testUsdExportRenderLayerMode.py
        testenv/testUsdExportSelection.py
        testenv/testUsdExportUVSets.py
        testenv/testUsdImportAsAssemblies.py
        testenv/testUsdImportCamera.py
        testenv/testUsdImportColorSets.py
        testenv/testUsdImportFrameRange.py
        testenv/testUsdImportNestedAssemblyAnimation.py
        testenv/testUsdImportSessionLayer.py
        testenv/testUsdImportUVSets.py
        testenv/testUsdMayaGetVariantSetSelections.py
        testenv/testUsdMayaModelKindWriter.py
        testenv/testUsdMayaProxyShape.py
        testenv/testUsdMayaUserExportedAttributes.py
        testenv/testUsdMetadataAttributeConverters.py
        testenv/testUsdReferenceAssemblyChangeRepresentations.py
        testenv/testUsdReferenceAssemblySelection.py
)

pxr_install_test_dir(
    SRC testenv/UsdExportAssemblyTest
    DEST testUsdExportAssembly
)
pxr_register_test(testUsdExportAssembly
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportAssembly"
    TESTENV testUsdExportAssembly
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportCameraTest
    DEST testUsdExportCamera
)
pxr_register_test(testUsdExportCamera
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportCamera"
    TESTENV testUsdExportCamera
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportColorSetsTest
    DEST testUsdExportColorSets
)
pxr_register_test(testUsdExportColorSets
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportColorSets"
    TESTENV testUsdExportColorSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportDisplayColorTest
    DEST testUsdExportDisplayColor
)
pxr_register_test(testUsdExportDisplayColor
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportDisplayColor"
    TESTENV testUsdExportDisplayColor
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportFrameOffsetTest
    DEST testUsdExportFrameOffset
)
pxr_register_test(testUsdExportFrameOffset
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportFrameOffset"
    TESTENV testUsdExportFrameOffset
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportMeshTest
    DEST testUsdExportMesh
)
pxr_register_test(testUsdExportMesh
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportMesh"
    TESTENV testUsdExportMesh
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportRenderLayerModeTest
    DEST testUsdExportRenderLayerMode
)
pxr_register_test(testUsdExportRenderLayerMode
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportRenderLayerMode"
    TESTENV testUsdExportRenderLayerMode
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportSelectionTest
    DEST testUsdExportSelection
)
pxr_register_test(testUsdExportSelection
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportSelection"
    TESTENV testUsdExportSelection
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdExportUVSetsTest
    DEST testUsdExportUVSets
)
pxr_register_test(testUsdExportUVSets
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdExportUVSets"
    TESTENV testUsdExportUVSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportAsAssemblies
    DEST testUsdImportAsAssemblies
)
pxr_register_test(testUsdImportAsAssemblies
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportAsAssemblies"
    TESTENV testUsdImportAsAssemblies
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportCameraTest
    DEST testUsdImportCamera
)
pxr_register_test(testUsdImportCamera
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportCamera"
    TESTENV testUsdImportCamera
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportColorSetsTest
    DEST testUsdImportColorSets
)
pxr_register_test(testUsdImportColorSets
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportColorSets"
    TESTENV testUsdImportColorSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportFrameRangeTest
    DEST testUsdImportFrameRange
)
pxr_register_test(testUsdImportFrameRange
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportFrameRange"
    TESTENV testUsdImportFrameRange
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportNestedAssemblyAnimationTest
    DEST testUsdImportNestedAssemblyAnimation
)
pxr_register_test(testUsdImportNestedAssemblyAnimation
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportNestedAssemblyAnimation"
    TESTENV testUsdImportNestedAssemblyAnimation
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportSessionLayerTest
    DEST testUsdImportSessionLayer
)
pxr_register_test(testUsdImportSessionLayer
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportSessionLayer"
    TESTENV testUsdImportSessionLayer
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdImportUVSetsTest
    DEST testUsdImportUVSets
)
pxr_register_test(testUsdImportUVSets
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdImportUVSets"
    TESTENV testUsdImportUVSets
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdMayaGetVariantSetSelectionsTest
    DEST testUsdMayaGetVariantSetSelections
)
pxr_register_test(testUsdMayaGetVariantSetSelections
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaGetVariantSetSelections"
    TESTENV testUsdMayaGetVariantSetSelections
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdMayaModelKindWriterTest
    DEST testUsdMayaModelKindWriter
)
pxr_register_test(testUsdMayaModelKindWriter
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaModelKindWriter"
    TESTENV testUsdMayaModelKindWriter
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdMayaProxyShape
    DEST testUsdMayaProxyShape
)
pxr_register_test(testUsdMayaProxyShape
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaProxyShape"
    TESTENV testUsdMayaProxyShape
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdMayaUserExportedAttributesTest
    DEST testUsdMayaUserExportedAttributes
)
pxr_register_test(testUsdMayaUserExportedAttributes
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdMayaUserExportedAttributes"
    TESTENV testUsdMayaUserExportedAttributes
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdMetadataAttributeConvertersTest
    DEST testUsdMetadataAttributeConverters
)
pxr_register_test(testUsdMetadataAttributeConverters
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdMetadataAttributeConverters"
    TESTENV testUsdMetadataAttributeConverters
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblyChangeRepresentationsTest
    DEST testUsdReferenceAssemblyChangeRepresentations
)
pxr_register_test(testUsdReferenceAssemblyChangeRepresentations
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdReferenceAssemblyChangeRepresentations"
    TESTENV testUsdReferenceAssemblyChangeRepresentations
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblySelectionTest
    DEST testUsdReferenceAssemblySelection
)
pxr_register_test(testUsdReferenceAssemblySelection
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testUsdReferenceAssemblySelection"
    TESTENV testUsdReferenceAssemblySelection
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)

