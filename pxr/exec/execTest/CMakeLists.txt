set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execTest)

pxr_build_test_library(execTest
    LIBRARIES
        arch
    PUBLIC_HEADERS
        api.h
    PUBLIC_CLASSES
        publicClass
)
