set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glfVdb)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

if (PXR_BUILD_OPENVDB_PLUGIN)
    list(APPEND optionalPublicClasses vdbTextureData)
    list(APPEND optionalLibs ${OPENEXR_Half_LIBRARY} ${OPENVDB_LIBRARY})
    list(APPEND optionalIncludeDirs ${OPENVDB_INCLUDE_DIR})
endif()

pxr_plugin(glfVdb
    LIBRARIES
        ar
        arch
        gf
        glf
        tf
        ${OPENEXR_Half_LIBRARY}
        ${OPENVDB_LIBRARY}

    INCLUDE_DIRS
        ${OPENVDB_INCLUDE_DIR}

    PUBLIC_CLASSES
        vdbTexture
        vdbTextureContainer
        vdbTextureData

    PRIVATE_CLASSES
        debugCodes

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapVdbTexture.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
