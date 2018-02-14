set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImaging)

# Note: both python include directives are included for compatibility.
pxr_library(usdImaging
    LIBRARIES
        gf
        tf
        plug
        tracelite
        vt
        work
        garch
        glf
        hd
        hdx
        pxOsd
        sdf
        usd
        usdGeom
        usdHydra
        usdLux
        usdRi
        usdShade
        ar
        ${Boost_PYTHON_LIBRARY}
        ${PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        adapterRegistry
        debugCodes
        delegate
        inheritedCache
        instancerContext
        package
        tokens
        unitTestHelper
        valueCache
        basisCurvesAdapter
        capsuleAdapter
        coneAdapter
        cubeAdapter
        cylinderAdapter
        domeLightAdapter
        drawModeAdapter
        gprimAdapter
        instanceAdapter
        lightAdapter
        materialAdapter
        meshAdapter
        nurbsPatchAdapter
        pointsAdapter
        pointInstancerAdapter
        primAdapter
        rectLightAdapter
        sphereAdapter
        sphereLightAdapter

    PUBLIC_HEADERS
        api.h
        version.h

    RESOURCE_FILES
        plugInfo.json
        shaders/drawMode.glslfx
)

