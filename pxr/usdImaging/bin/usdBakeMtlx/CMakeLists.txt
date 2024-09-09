set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdBakeMtlx)

# MaterialX's MaterialXRenderGlsl library, which is used by usdBakeMtlx,
# requires OpenGL.
if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_library(usdBakeMtlx
    LIBRARIES
        tf
        sdr
        usdMtlx
        usdShade
        hd
        hdMtlx
        usdImaging
        MaterialXCore
        MaterialXFormat
        MaterialXRenderGlsl

    PUBLIC_CLASSES
        bakeMaterialX

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBakeMaterialX.cpp

    PYMODULE_FILES
        __init__.py

    DISABLE_PRECOMPILED_HEADERS
)

pxr_python_bin(usdBakeMaterialX
    DEPENDENCIES
        usdBakeMtlx
)
