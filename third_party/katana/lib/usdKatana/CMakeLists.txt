set(PXR_PACKAGE usdKatana)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        plug
        gf
        tf
        tracelite
        vt
        ar
        sdf
        usd
        usdImaging
        usdImagingGL
        usdShade
        usdGeom
        usdRi
        usdUI
        usdUtils
        cameraUtil
        katanaAttrfncApi
        ${Boost_SYSTEM_LIBRARY} 
        ${Boost_THREAD_LIBRARY} 

    PUBLIC_CLASSES
        attrMap
        baseMaterialHelpers
        blindDataObject
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
        wrapLookAPI.cpp
        module.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

