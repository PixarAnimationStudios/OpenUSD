set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE ar)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(ar
    LIBRARIES
        arch
        tf
        plug
        vt
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h
        assetInfo.h
        pyResolverContext.h
        resolver.h
        resolverContext.h
        resolverContextBinder.h
        resolverScopedCache.h
        defineResolver.h
        defineResolverContext.h

    PRIVATE_HEADERS
        debugCodes.h
        defaultResolver.h

    CPPFILES
        assetInfo.cpp
        pyResolverContext.cpp
        resolver.cpp
        resolverContext.cpp
        resolverContextBinder.cpp
        resolverScopedCache.cpp
        debugCodes.cpp
        defaultResolver.cpp
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapResolver.cpp
        wrapResolverContext.cpp
        wrapResolverContextBinder.cpp
        wrapResolverScopedCache.cpp

    PYTHON_FILES
        __init__.py
        plugInfo.json
)

