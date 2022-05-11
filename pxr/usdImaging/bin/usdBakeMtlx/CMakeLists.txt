if (WIN32)
    message(WARNING "UsdBakeMtlx is not supported on Windows")
    return()
endif()

set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdBakeMtlx)

pxr_library(usdBakeMtlx
    LIBRARIES
        tf
        sdr
        usdMtlx
        usdShade
        hd
        hdMtlx
        usdImaging
        ${MATERIALX_LIBRARIES}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MATERIALX_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}

    PUBLIC_CLASSES
        bakeMaterialX

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