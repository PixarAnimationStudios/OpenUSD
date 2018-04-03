set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdSkel)

pxr_library(usdSkel
    LIBRARIES
        arch
        gf
        tf
        trace
        vt
        work
        sdf
        usd
        usdGeom
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        animMapper
        animQuery
        bindingAPI
        cache
        debugCodes
        packedJointAnimation
        root
        skeleton
        skeletonQuery
        skinningQuery
        tokens
        topology
        utils

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        animQueryImpl
        cacheImpl
        skelDefinition

    PYTHON_CPPFILES
        moduleDeps.cpp    

    PYMODULE_FILES
        __init__.py

    PYMODULE_CPPFILES
        module.cpp
        wrapAnimMapper.cpp
        wrapAnimQuery.cpp
        wrapBindingAPI.cpp
        wrapCache.cpp
        wrapPackedJointAnimation.cpp
        wrapRoot.cpp
        wrapSkeleton.cpp
        wrapSkeletonQuery.cpp
        wrapSkinningQuery.cpp
        wrapTokens.cpp
        wrapTopology.cpp
        wrapUtils.cpp

    RESOURCE_FILES
        generatedSchema.usda
        plugInfo.json
        schema.usda:usdSkel/schema.usda
)
   
