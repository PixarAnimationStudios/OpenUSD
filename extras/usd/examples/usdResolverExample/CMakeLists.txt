set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdResolverExample)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        js
        tf
        vt

    PUBLIC_CLASSES
        resolver
        resolverContext

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapResolverContext.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
    testenv/testUsdResolverExample.py
)

pxr_install_test_dir(
    SRC testenv/testUsdResolverExample
    DEST testUsdResolverExample
)

pxr_register_test(testUsdResolverExample
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdResolverExample"
    EXPECTED_RETURN_CODE 0
    ENV
        ${PXR_PLUGINPATH_NAME}=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdResolverExample/resources
        USD_RESOLVER_EXAMPLE_ASSET_DIR=./assets
)
    
