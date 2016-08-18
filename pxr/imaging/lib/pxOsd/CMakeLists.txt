set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE pxOsd)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(pxOsd
    LIBRARIES
        tf
        gf
        vt
        ${OPENSUBDIV_LIBRARIES}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        meshTopology
        refinerFactory
        refinerCache
        stencilPerVertex
        subdivTags
        tokens
        uniformEvaluator

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}

)
