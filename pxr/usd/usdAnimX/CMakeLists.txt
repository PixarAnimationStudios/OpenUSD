set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAnimX)

pxr_library(${PXR_PACKAGE}
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
        keyframe
        curve
        
    PUBLIC_HEADERS
        api.h
        tokens.h
        types.h
        animx.h
        desc.h
        keyframe.h
        interpolation.h
        data.h

    PRIVATE_CLASSES
        tokens
        types
        animx
        data
        reader
        writer

    RESOURCE_FILES
        plugInfo.json
        animX.usda:usdAnimX/animX.usda
        empty.animx:usdAnimX/empty.animx
)
