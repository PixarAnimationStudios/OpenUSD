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
        ${MAYA_OpenMayaAnim_LIBRARY}
        ${MAYA_OpenMayaFX_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_CLASSES
        debugCodes
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
        translatorLook
        translatorMesh
        translatorModelAssembly
        translatorNurbsPatch
        translatorPrim
        translatorUtil
        translatorXformable
        shadingModeExporter
        shadingModeImporter
        shadingModeRegistry
        util
        usdPrimProvider
        writeUtil

        MayaPrimWriter
        JobArgs
        usdReadJob
        usdWriteJob

        pluginStaticData
        proxyShape
        referenceAssembly
        usdImport
        usdExport
        usdTranslatorExport
        usdTranslatorImport

        Chaser
        ChaserRegistry
        
        AttributeConverter
        AttributeConverterRegistry

        UserTaggedAttribute

    PRIVATE_CLASSES
        registryHelper
        MayaCameraWriter
        MayaMeshWriter
        MayaNurbsCurveWriter
        MayaNurbsSurfaceWriter
        MayaTransformWriter
        PluginPrimWriter

    PRIVATE_HEADERS
        shadingModePxrRis_rfm_map.h

    CPPFILES
        MayaMeshWriter_PrimVars.cpp
        MayaMeshWriter_SubDiv.cpp
        primReaderCamera.cpp
        primReaderLook.cpp
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
        wrapStageCache.cpp 
        wrapUserTaggedAttribute.cpp 

    PYTHON_FILES
        __init__.py 
        AEpxrUsdReferenceAssemblyTemplate.py 
        userExportedAttributesUI.py 

    RESOURCE_FILES
        AEpxrUsdProxyShapeTemplate.mel 
        usdMaya.mel 
        usdTranslatorExport.mel 
        usdTranslatorImport.mel 
)

