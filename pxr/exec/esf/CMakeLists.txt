set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE esf)

pxr_library(esf
    LIBRARIES
        arch
        sdf
        tf
        vt
        usd
        
    PUBLIC_HEADERS
        api.h
        journal.h
        schemaConfigKey.h

    PUBLIC_CLASSES
        attribute
        attributeQuery
        editReason
        fixedSizePolymorphicHolder
        object
        prim
        property
        relationship
        stage

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md
)

pxr_build_test(testEsfFixedSizePolymorphicHolder
    LIBRARIES
        esf
        tf
    CPPFILES
        testenv/testEsfFixedSizePolymorphicHolder.cpp
)

pxr_build_test(testEsfEditReason
    LIBRARIES
        esf
    CPPFILES
        testenv/testEsfEditReason.cpp
)

pxr_register_test(testEsfFixedSizePolymorphicHolder
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfFixedSizePolymorphicHolder"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testEsfEditReason
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfEditReason"
    EXPECTED_RETURN_CODE 0
)
