set(PXR_PACKAGE usdKatana)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        plug
        gf
        tf
        trace
        vt
        ar
        sdf
        usd
        usdHydra
        usdImaging
        usdImagingGL
        usdShade
        usdSkel
        usdGeom
        usdLux
        usdRi
        usdUI
        usdUtils
        cameraUtil
        katanaAttrfncApi
        katanaOpApi
        katanaPluginApi
        ${Boost_CHRONO_LIBRARY} 
        ${Boost_DATE_TIME_LIBRARY} 
        ${Boost_SYSTEM_LIBRARY} 
        ${Boost_THREAD_LIBRARY} 

    PUBLIC_HEADERS
        api.h
        
    PUBLIC_CLASSES
        attrMap
        baseMaterialHelpers
        blindDataObject
        bootstrap
        cache
        debugCodes
        locks
        tokens
        lookAPI
        utils

        usdInArgs
        usdInPrivateData
        usdInPluginRegistry

        readBasisCurves
        readBlindData
        readCamera
        readConstraintTarget
        readGprim
        readGeomSubset
        readLight
        readLightFilter
        readMaterial
        readMesh
        readModel
        readNurbsPatch
        readPointInstancer
        readPoints
        readPrim
        readXformable

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapBlindDataObject.cpp
        wrapCache.cpp
        wrapLookAPI.cpp
        module.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdKatana/schema.usda
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
