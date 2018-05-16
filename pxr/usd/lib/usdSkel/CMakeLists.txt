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
        blendShape
        cache
        debugCodes
        inbetweenShape
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
        wrapBlendShape.cpp
        wrapCache.cpp
        wrapInbetweenShape.cpp
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
   
pxr_test_scripts(
    testenv/testUsdSkelAnimMapper.py
    testenv/testUsdSkelAnimQuery.py 
    testenv/testUsdSkelBindingAPI.py
    testenv/testUsdSkelCache.py
    testenv/testUsdSkelRoot.py
    testenv/testUsdSkelSkeletonQuery.py
    testenv/testUsdSkelSkinningQuery.py
    testenv/testUsdSkelTopology.py
    testenv/testUsdSkelUtils.py
)

pxr_install_test_dir(
    SRC testenv/testUsdSkelCache
    DEST testUsdSkelCache
)

pxr_install_test_dir(
    SRC testenv/testUsdSkelRoot
    DEST testUsdSkelRoot
)

pxr_install_test_dir(
    SRC testenv/testUsdSkelSkinningQuery
    DEST testUsdSkelSkinningQuery
)

pxr_register_test(testUsdSkelAnimMapper
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelAnimMapper"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelAnimQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelAnimQuery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelBindingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelBindingAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelCache
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelCache"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelRoot
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelRoot"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelSkeletonQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelSkeletonQuery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelSkinningQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelSkinningQuery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelTopology
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelTopology"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSkelUtils
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelUtils"
    EXPECTED_RETURN_CODE 0
)
