set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execTest)

pxr_build_test_library(execTest
    LIBRARIES
        arch
        tf
    PUBLIC_HEADERS
        api.h
    PUBLIC_CLASSES
        publicClass
    CPPFILES
        registration.cpp
    RESOURCE_FILES
        plugInfo.json
    
    INSTALL_PREFIX ExecTestPlugins
)
