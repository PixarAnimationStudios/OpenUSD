set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdDancingCubesExample)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        gf
        vt
        pcp
        sdf
        usd

    PUBLIC_CLASSES
        fileFormat

    PRIVATE_CLASSES
        data
        dataImpl

    RESOURCE_FILES
        plugInfo.json
        dancingCubes.usda:usdDancingCubesExample/dancingCubes.usda
        empty.usddancingcubesexample:usdDancingCubesExample/empty.usddancingcubesexample

    DISABLE_PRECOMPILED_HEADERS
)

