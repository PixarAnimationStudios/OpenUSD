set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAnimX)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        arch
        tf
        gf
        trace
        vt
        work
        pcp
        sdf
        usd
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    PUBLIC_CLASSES
        fileFormat
        
    PUBLIC_HEADERS
        api.h
        tokens.h
        types.h
        animx.h
        desc.h
        keyframe.h

    PRIVATE_CLASSES
        tokens
        types
        animx
        keyframe
        curve
        data
        reader

    RESOURCE_FILES
        plugInfo.json
        animX.usda:usdAnimX/animX.usda
        empty.animx:usdAnimX/empty.animx
)
