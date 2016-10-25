set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE plug)

pxr_shared_library(plug
    LIBRARIES
        arch
        tf
        js
        tracelite
        work
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}
        ${TBB_tbbmalloc_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        interfaceFactory
        notice
        plugin
        registry
        staticInterface

    PRIVATE_CLASSES
        debugCodes
        info
        testPlugBase

    PUBLIC_HEADERS
        api.h
        thisPlugin.h

    CPPFILES
        initConfig.cpp
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapNotice.cpp
        wrapPlugin.cpp
        wrapRegistry.cpp
        wrapTestPlugBase.cpp

    PYTHON_FILES
        __init__.py
)

pxr_test_scripts(
    testPlug.py
)

pxr_build_test_shared_lib(TestPlugDso1
    CREATE_FRAMEWORK
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDso1.cpp
) 
pxr_build_test_shared_lib(TestPlugDso2
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDso2.cpp
) 
pxr_build_test_shared_lib(TestPlugDso3
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDso3.cpp
) 
pxr_build_test_shared_lib(TestPlugDsoEmpty
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDsoEmpty.cpp
) 
pxr_build_test_shared_lib(TestPlugDsoIncomplete
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDsoIncomplete.cpp
) 
pxr_build_test_shared_lib(TestPlugDsoUnloadable
    LIBRARIES
        tf
    CPPFILES
        testenv/TestPlugDsoUnloadable.cpp
) 
# This library helps test behavior when a plugin is unloadable,
# but clang by default will error out when building this due to
# the references to undefined symbols. So we need to specify a
# link flag to force these symbols to be looked up at runtime.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set_target_properties(TestPlugDsoUnloadable
        PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

pxr_create_test_module(TestPlugModule1
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModule2
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModule3
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleEmpty
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleLoaded
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleLoadedBadBase
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleUnloadable
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleUnloadable2
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleDepBadBase
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleDepBadDep
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleDepBadDep2
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleDepBadLoad
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleDepCycle
    INSTALL_PREFIX PlugPlugins
)
pxr_create_test_module(TestPlugModuleIncomplete
    INSTALL_PREFIX PlugPlugins
)

pxr_register_test(testPlug
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPlug"
)

