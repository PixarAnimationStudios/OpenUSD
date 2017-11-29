
set(PXR_PACKAGE gusd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HOUDINI_DEFINITIONS}")

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        ar 
        gf 
        sdf 
        tf
        usd
        usdGeom
        usdRi
        usdShade
        usdUtils

    INCLUDE_DIRS
        ${HOUDINI_INCLUDE_DIRS}

    PUBLIC_CLASSES
        GT_Utils
        GU_PackedUSD
        GU_USD
        OP_ParmChangeMicroNode
        primWrapper
        PRM_Shared
        purpose
        refiner
        scopeWrapper
        shaderWrapper
        stageCache
        stageEdit
        tokens
        GT_PrimCache
        GT_PointInstancer
        GT_OldPointInstancer
        USD_CustomTraverse
        USD_DataCache
        USD_StdTraverse
        USD_ThreadedTraverse
        USD_Traverse
        USD_Utils
        USD_XformCache
        USD_VisCache
        UT_Error
        UT_TypeTraits
        visitor
        groupBaseWrapper
        xformWrapper
        writeCtrlFlags
        curvesWrapper
        NURBSCurvesWrapper

    PUBLIC_HEADERS
        context.h
        api.h
        defaultArray.h
        gusd.h
        GU_USD.h
        stageOpts.h
        USD_PropertyMap.h
        USD_TraverseSimple.h
        UT_Assert.h
        UT_CappedCache.h
        UT_StaticInit.h
        UT_Version.h
        UT_Gf.h
        GT_VtArray.h

    PRIVATE_HEADERS
        GT_VtStringArray.h
        UT_VtArray.h

    CPPFILES
        plugin.cpp

    PRIVATE_CLASSES
        GEO_IOTranslator
        GT_PackedUSD
        boundsCache
        meshWrapper
        packedUsdWrapper
        pointsWrapper
        instancerWrapper
)

file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "site/icons/*")

install( 
    FILES
        ${ICON_FILES}
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/config/Icons
)


install( 
    FILES
        __init__.py
        treeview.py
        treemodel.py       
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/scripts/python/gusd
)
