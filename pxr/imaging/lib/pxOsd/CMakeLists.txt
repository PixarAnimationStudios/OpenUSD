set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE pxOsd)

pxr_library(pxOsd
    LIBRARIES
        tf
        gf
        vt
        ${OPENSUBDIV_LIBRARIES}
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        meshTopology
        refinerFactory
        subdivTags
        tokens

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapMeshTopology.cpp
        wrapSubdivTags.cpp

    PYMODULE_FILES
        __init__.py
)
