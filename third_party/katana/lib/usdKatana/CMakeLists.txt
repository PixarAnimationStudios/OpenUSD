set(PXR_PACKAGE usdKatana)

# warning C4838: conversion from 'double' to 'float' requires a
# narrowing conversion
add_definitions("/wd4838")

add_definitions(
    /DFNASSET_STATIC
    /DFNATTRIBUTE_STATIC
	/DFNATTRIBUTEFUNCTION_STATIC
    /DFNATTRIBUTEMODIFIER_STATIC
    /DFNCONFIG_STATIC
    /DFNDEFAULTATTRIBUTEPRODUCER_STATIC
	/DFNGEOLIB_STATIC
    /DFNGEOLIBSERVICES_STATIC
    /DFNLOGGING_STATIC
    /DFNRENDEROUTPUTLOCATION_STATIC
    /DFNSCENEGRAPHGENERATOR_STATIC
    /DFNSCENEGRAPHITERATOR_STATIC
    /DPYSTRING_STATIC
)

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
        usdShade
        usdGeom
        usdRi
        usdUtils
        cameraUtil
        katanaAttrfncApi
		katanaOpApi
		katanaPluginApi
        ${Boost_SYSTEM_LIBRARY} 
        ${Boost_THREAD_LIBRARY} 

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        attrMap
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
        readLook
        readMesh
        readModel
        readNurbsPatch
        readPoints
        readPrim
        readXformable

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

