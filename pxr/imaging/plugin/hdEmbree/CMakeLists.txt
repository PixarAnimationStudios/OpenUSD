set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdEmbree)

if(NOT EMBREE_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Embree")
    return()
endif()

pxr_plugin(hdEmbree
   LIBRARIES
        plug
        tf
        vt
        gf
        work
        hf
        hd
        hdx
        ${TBB_LIBRARIES}
        ${EMBREE_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        ${EMBREE_INCLUDE_DIR}

    PUBLIC_CLASSES
        config
        instancer
        mesh
        rendererPlugin
        renderDelegate
        renderPass

    PUBLIC_HEADERS
        renderParam.h

    RESOURCE_FILES
        plugInfo.json
)

if (X11_FOUND OR APPLE)
pxr_build_test(testHdEmbree
    LIBRARIES
        hdEmbree
        garch
        arch
    CPPFILES
        testenv/testHdEmbree.cpp
)
endif()
