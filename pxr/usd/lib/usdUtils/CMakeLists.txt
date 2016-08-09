set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUtils)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(usdUtils
    LIBRARIES
        arch
        tf
        gf
        sdf
        usd
        usdGeom
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        authoring
        dependencies
        introspection
        pipeline
        registeredVariantSet
        stageCache
        stitch
        stitchClips

    PUBLIC_HEADERS
        api.h
        
    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAuthoring.cpp
        wrapDependencies.cpp
        wrapIntrospection.cpp
        wrapPipeline.cpp
        wrapRegisteredVariantSet.cpp
        wrapStageCache.cpp
        wrapStitch.cpp
        wrapStitchClips.cpp

    PYTHON_FILES
        __init__.py
)
