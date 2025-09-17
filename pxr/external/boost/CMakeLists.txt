set(PXR_PREFIX pxr/external)
set(PXR_PACKAGE boost)

# This subdirectory currently only contains Python-related
# libraries, so skip it entirely if Python support is disabled.
if (NOT PXR_ENABLE_PYTHON_SUPPORT AND NOT TARGET python_modules)
    return()
endif()

pxr_library(boost
    PUBLIC_HEADERS
        python.hpp

    CPPFILES
        # Workaround lack of build support for header-only libs.
        build-workaround.cpp

    DISABLE_PRECOMPILED_HEADERS
)

# Ensure symbol in build-workaround.cpp is exported on Windows,
# otherwise the build will error out at link time because no
# .lib file will be generated. Using this setting is OK because
# we only expect there to be one exportable symbol in this library.
set_target_properties(boost 
    PROPERTIES 
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

add_subdirectory(python)
