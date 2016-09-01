#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
include(Private)

function(pxr_python_bins)
    _get_install_dir(bin installDir)
    foreach(file ${ARGN})
        set(pyFile ${file}.py)

        # /pxrpythonsubst will be replaced with the full path to the configured
        # python executable. This doesn't use the CMake ${...} or @...@ syntax
        # for backwards compatibility with other build systems.
        file(READ ${pyFile} contents)
        string(REGEX REPLACE "/pxrpythonsubst" ${PXR_PYTHON_SHEBANG} 
            contents "${contents}")
        file(WRITE ${CMAKE_BINARY_DIR}/${pyFile} ${contents})

        install(PROGRAMS
            ${CMAKE_BINARY_DIR}/${pyFile}
            DESTINATION ${installDir}
            RENAME ${file}
        )

        # Windows by default cannot execute Python files so here
        # we create a batch file wrapper that invokes the python
        # files.
        if(WIN32)
            file(WRITE "${CMAKE_BINARY_DIR}/${pyFile}.cmd" "@set PATH=C:\\Program Files\\usd\\lib;%PATH%\r\n@python \"%~dp0${file}\" %*")
            install(PROGRAMS
                "${CMAKE_BINARY_DIR}/${pyFile}.cmd"
                DESTINATION ${installDir}
                RENAME "${file}.cmd"
            )
        endif()
    endforeach()
endfunction() # pxr_install_python_bins

function(pxr_cpp_bin BIN_NAME)
    _get_install_dir(bin installDir)
    
    set(multiValueArgs
        LIBRARIES
        INCLUDE_DIRS
    )

    cmake_parse_arguments(cb
        ""  
        ""
        "${multiValueArgs}"
        ${ARGN}
    )

    add_executable(${BIN_NAME} ${BIN_NAME}.cpp)
    add_dependencies(${BIN_NAME} ${cb_LIBRARIES})

    # Install and include headers from the build directory.
    get_filename_component(
        PRIVATE_INC_DIR
        "${CMAKE_BINARY_DIR}/include"
        ABSOLUTE
    )

    target_include_directories(${BIN_NAME}
        PRIVATE 
        ${cb_INCLUDE_DIRS}
        ${PRIVATE_INC_DIR}
    )

    set_target_properties(${BIN_NAME}
        PROPERTIES 
            INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    if (PXR_MALLOC_LIBRARY)
        target_link_libraries(${BIN_NAME} ${cb_LIBRARIES})
    else()
        target_link_libraries(${BIN_NAME}
            ${cb_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
        )
    endif()

    install(TARGETS 
        ${BIN_NAME}
        DESTINATION ${installDir}
    )

endfunction()

function(pxr_add_filename_property LIBRARY_NAME FILELIST)
    foreach(cppfile ${FILELIST})

        get_filename_component(fileExt "${cppfile}" EXT)

        if("${fileExt}" STREQUAL "")
            get_filename_component(
                cppModuleName
                "${cppfile}"
                NAME_WE)

            set_source_files_properties(${cppfile}.cpp PROPERTIES COMPILE_FLAGS
                                -D__FILENAME__=${LIBRARY_NAME}${cppModuleName})

        elseif(fileExt MATCHES .cpp)
            get_filename_component(
                cppModuleName
                "${cppfile}"
                NAME_WE)

            set_source_files_properties(${cppfile} PROPERTIES COMPILE_FLAGS
                                -D__FILENAME__=${LIBRARY_NAME}${cppModuleName})
        endif()
    endforeach()
endfunction()

function(pxr_shared_library LIBRARY_NAME)
    set(options PYTHON_LIBRARY)
    set(multiValueArgs
        PUBLIC_CLASSES
        PUBLIC_HEADERS
        PRIVATE_CLASSES
        PRIVATE_HEADERS
        CPPFILES
        PYMODULE_CPPFILES
        PYTHON_FILES
        PYSIDE_UI_FILES
        LIBRARIES
        INCLUDE_DIRS
        RESOURCE_FILES
    )

    cmake_parse_arguments(sl
        "${options}"
        ""
        "${multiValueArgs}"
        ${ARGN}
    )

    _classes(${LIBRARY_NAME} ${sl_PRIVATE_CLASSES} PRIVATE)
    _classes(${LIBRARY_NAME} ${sl_PUBLIC_CLASSES} PUBLIC)

    set(PXR_ALL_LIBS
        "${PXR_ALL_LIBS} ${LIBRARY_NAME}"
        CACHE
        INTERNAL
        "Aggregation of all internal libraries."
    )

    add_library(${LIBRARY_NAME}
        SHARED
        ${sl_CPPFILES} ${${LIBRARY_NAME}_CPPFILES}
        ${sl_PUBLIC_HEADERS} ${${LIBRARY_NAME}_PUBLIC_HEADERS}
        ${sl_PRIVATE_HEADERS} ${${LIBRARY_NAME}_PRIVATE_HEADERS}
    )

    if(sl_CPPFILES)
        pxr_add_filename_property(${LIBRARY_NAME} "${sl_CPPFILES}")
    endif()
    if(sl_PRIVATE_CLASSES)
        pxr_add_filename_property(${LIBRARY_NAME} "${sl_PRIVATE_CLASSES}")
    endif()
    if(sl_PUBLIC_CLASSES)
        pxr_add_filename_property(${LIBRARY_NAME} "${sl_PUBLIC_CLASSES}")
    endif()

    if(WIN32)
        # If we're building a DLL we want to add a module entry point to it.
        # We don't do this for Python PYDs though.
        if (NOT sl_PYTHON_LIBRARY)
            target_sources(${LIBRARY_NAME} PRIVATE "DllMain.cpp")
        endif()

        if(MSVC)
            set_target_properties(
                ${LIBRARY_NAME}
                PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
        endif()
    endif()

    if(sl_PYTHON_FILES)
        _install_python(${LIBRARY_NAME}
            FILES ${sl_PYTHON_FILES}
        )
    endif()

    # Convert the name of the library into the python module name
    # , e.g. _tf.so -> Tf. This is later used to determine the eventual
    # install location as well as for inclusion into the __init__.py's 
    # __all__ list.
    _get_python_module_name(${LIBRARY_NAME} pyModuleName)

    # If we are building a python library, we want it to have the name
    # _foo.so and install to ${project}/lib/python/${project}/${libname}
    if(sl_PYTHON_LIBRARY)
        # Always install under the 'pxr' module, rather than base on the
        # project name. This makes importing consistent, e.g. 
        # 'from pxr import X'. Additionally, python libraries always install
        # into the default lib install, not into the third_party subdirectory
        # or similar.
        set(LIB_INSTALL_PREFIX "lib/python/pxr/${pyModuleName}")
        
        set_property(GLOBAL
            APPEND PROPERTY PXR_PYTHON_MODULES ${pyModuleName}
        )

        # Python modules for third_party libs are installed into the root
        # pxr/lib/python but need to be able to access their corresponding
        # library which lives in third_party/${pkg}/lib
        set(rpath ${CMAKE_INSTALL_RPATH})
        if (PXR_INSTALL_SUBDIR)
            set(rpath "$ORIGIN/../../../../${PXR_INSTALL_SUBDIR}/lib:${rpath}")
        endif()

        if(WIN32)
			add_definitions(-D_BUILDING_PYD=1)
            set_target_properties(${LIBRARY_NAME} 
                PROPERTIES 
                    PREFIX ""
                    SUFFIX ".pyd"
                    FOLDER "${PXR_PREFIX}/_python"
                    LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
            )
        else()
            set_target_properties(${LIBRARY_NAME} 
                PROPERTIES 
                    PREFIX ""
                    SUFFIX ".pyd"
                    FOLDER "${PXR_PREFIX}/_python"
                    INSTALL_RPATH ${rpath}
            )
        endif()
    else()
        _get_install_dir(lib LIB_INSTALL_PREFIX)
        _get_share_install_dir(SHARE_INSTALL_PREFIX)

        set(PLUGINS_PREFIX ${SHARE_INSTALL_PREFIX}/plugins)

        set_target_properties(${LIBRARY_NAME}
            PROPERTIES
                FOLDER "${PXR_PREFIX}"
        )
    endif()

    if(PXR_INSTALL_SUBDIR)
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/${PXR_INSTALL_SUBDIR}/include/${PXR_PREFIX}/${LIBRARY_NAME}")
    else()
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/include/${PXR_PREFIX}/${LIBRARY_NAME}")
    endif()

    if(PXR_INSTALL_LOCATION)
        set(installLocation ${PXR_INSTALL_LOCATION})
    else()
        set(installLocation ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PREFIX})
    endif()

	if(WIN32)
		set(_PxrUserLocation "C:/ProgramData/usd/plugins")
	else()
		set(_PxrUserLocation "/usr/local/share/usd/plugins")
	endif()
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES COMPILE_DEFINITIONS 
            "MFB_PACKAGE_NAME=${PXR_PACKAGE};MFB_ALT_PACKAGE_NAME=${PXR_PACKAGE};MFB_PACKAGE_MODULE=${pyModuleName};PXR_USER_LOCATION=${_PxrUserLocation};PXR_BUILD_LOCATION=${CMAKE_INSTALL_PREFIX}/${PLUGINS_PREFIX};PXR_INSTALL_LOCATION=${installLocation}"
    )

    # Always bake the rpath.
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    _install_headers(${LIBRARY_NAME}
        FILES
            ${sl_PUBLIC_HEADERS}
            ${sl_PRIVATE_HEADERS}
            ${${LIBRARY_NAME}_PUBLIC_HEADERS}
            ${${LIBRARY_NAME}_PRIVATE_HEADERS}
        PREFIX ${PXR_PREFIX}
    )

    string(TOUPPER ${LIBRARY_NAME} ucLibName)

    set_target_properties(${LIBRARY_NAME}
        PROPERTIES
            PUBLIC_HEADER
                "${sl_PUBLIC_HEADERS};${${LIBRARY_NAME}_PUBLIC_HEADERS}"
            INTERFACE_INCLUDE_DIRECTORIES 
                ""
            DEFINE_SYMBOL
                "${ucLibName}_EXPORTS"
    )

    # Install and include headers from the build directory.
    get_filename_component(
        PRIVATE_INC_DIR
        "${CMAKE_BINARY_DIR}/include"
        ABSOLUTE
    )
    target_include_directories(${LIBRARY_NAME}
        PRIVATE ${PRIVATE_INC_DIR}
    )

    # Allow #include'ing of headers within the same install subdir.
    if (PXR_INSTALL_SUBDIR)
        get_filename_component(
            SUBDIR_INC_DIR
            "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}/include"
            ABSOLUTE
        )

        target_include_directories(${LIBRARY_NAME}
            PRIVATE ${SUBDIR_INC_DIR}
        )
    endif()

    install(TARGETS ${LIBRARY_NAME}
        EXPORT pxrTargets
		ARCHIVE DESTINATION ${LIB_INSTALL_PREFIX}
        LIBRARY DESTINATION ${LIB_INSTALL_PREFIX}
        ARCHIVE DESTINATION ${LIB_INSTALL_PREFIX}
        RUNTIME DESTINATION ${LIB_INSTALL_PREFIX}
        PUBLIC_HEADER DESTINATION ${HEADER_INSTALL_PREFIX}
    )

    export(TARGETS ${LIBRARY_NAME}
        APPEND
        FILE "${PROJECT_BINARY_DIR}/pxrTargets.cmake"
    )
    
    if (PXR_MALLOC_LIBRARY) 
        target_link_libraries(${LIBRARY_NAME}
            ${sl_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
        )
    else()
        target_link_libraries(${LIBRARY_NAME}
            ${sl_LIBRARIES}
        )
    endif() 

    # Include system headers before our own.  We define several headers
    # that conflict; for example, half.h in EXR versus gf
    if (sl_INCLUDE_DIRS)
        target_include_directories(${LIBRARY_NAME}
            BEFORE
            PUBLIC
            ${sl_INCLUDE_DIRS}
        )
    endif()

    # Build python module.
    if(DEFINED sl_PYMODULE_CPPFILES)
        pxr_shared_library(
            "_${LIBRARY_NAME}"
            PYTHON_LIBRARY
            CPPFILES ${sl_PYMODULE_CPPFILES}
            LIBRARIES ${LIBRARY_NAME}
        )
    endif()

    if (sl_RESOURCE_FILES)
        _install_resource_files(${sl_RESOURCE_FILES})
    endif()

    if (sl_PYSIDE_UI_FILES)
        _install_pyside_ui_files(${sl_PYSIDE_UI_FILES})
    endif()        

endfunction() # pxr_shared_library

function(pxr_static_library LIBRARY_NAME)
    set(multiValueArgs
        PUBLIC_CLASSES
        PUBLIC_HEADERS
        PRIVATE_CLASSES
        PRIVATE_HEADERS
        CPPFILES
        LIBRARIES
        INCLUDE_DIRS
    )

    cmake_parse_arguments(sl
        "${options}"
        ""
        "${multiValueArgs}"
        ${ARGN}
    )

    _classes(${LIBRARY_NAME} ${sl_PRIVATE_CLASSES} PRIVATE)
    _classes(${LIBRARY_NAME} ${sl_PUBLIC_CLASSES} PUBLIC)

    set(PXR_ALL_LIBS
        "${PXR_ALL_LIBS} ${LIBRARY_NAME}"
        CACHE
        INTERNAL
        "Aggregation of all internal libraries."
    )

    add_library(${LIBRARY_NAME}
        STATIC
        ${sl_CPPFILES} ${${LIBRARY_NAME}_CPPFILES}
        ${sl_PUBLIC_HEADERS} ${${LIBRARY_NAME}_PUBLIC_HEADERS}
        ${sl_PRIVATE_HEADERS} ${${LIBRARY_NAME}_PRIVATE_HEADERS}
    )

    # Even though this library is static, still want to build with -fPIC
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES POSITION_INDEPENDENT_CODE ON
    )

    if(PXR_INSTALL_SUBDIR)
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/${PXR_INSTALL_SUBDIR}/include/${PXR_PREFIX}/${LIBRARY_NAME}")
    else()
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/include/${PXR_PREFIX}/${LIBRARY_NAME}")
    endif()

    set_target_properties(${LIBRARY_NAME}
        PROPERTIES COMPILE_DEFINITIONS 
            "MFB_PACKAGE_NAME=${PXR_PACKAGE};MFB_ALT_PACKAGE_NAME=${PXR_PACKAGE}"
    )

    # Always bake the rpath.
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    _install_headers(${LIBRARY_NAME}
        FILES
            ${sl_PUBLIC_HEADERS}
            ${sl_PRIVATE_HEADERS}
            ${${LIBRARY_NAME}_PUBLIC_HEADERS}
            ${${LIBRARY_NAME}_PRIVATE_HEADERS}
        PREFIX ${PXR_PREFIX}
    )

    set_target_properties(${LIBRARY_NAME}
        PROPERTIES
            PUBLIC_HEADER
                "${sl_PUBLIC_HEADERS};${${LIBRARY_NAME}_PUBLIC_HEADERS}"
            INTERFACE_INCLUDE_DIRECTORIES ""
    )
# Install and include headers from the build directory.
    get_filename_component(
        PRIVATE_INC_DIR
        "${CMAKE_BINARY_DIR}/include"
        ABSOLUTE
    )
    target_include_directories(${LIBRARY_NAME}
        PRIVATE ${PRIVATE_INC_DIR}
    )

    _get_install_dir(lib LIB_INSTALL_PREFIX)

    # Allow #include'ing of headers within the same install subdir.
    if (PXR_INSTALL_SUBDIR)
        get_filename_component(
            SUBDIR_INC_DIR
            "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}/include"
            ABSOLUTE
        )

        target_include_directories(${LIBRARY_NAME}
            PRIVATE ${SUBDIR_INC_DIR}
        )
    endif()

    install(TARGETS ${LIBRARY_NAME}
        EXPORT pxrTargets
        ARCHIVE DESTINATION ${LIB_INSTALL_PREFIX}
        PUBLIC_HEADER DESTINATION ${HEADER_INSTALL_PREFIX}
    )

    export(TARGETS ${LIBRARY_NAME}
        APPEND
        FILE "${PROJECT_BINARY_DIR}/pxrTargets.cmake"
    )

    if (PXR_MALLOC_LIBRARY)
        target_link_libraries(${LIBRARY_NAME}
            ${sl_LIBRARIES}
        )
    else()
        target_link_libraries(${LIBRARY_NAME}
            ${sl_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
        )
    endif()

    # Include system headers before our own.  We define several headers
    # that conflict; for example, half.h in EXR versus gf
    if (sl_INCLUDE_DIRS)
        target_include_directories(${LIBRARY_NAME}
            BEFORE
            PUBLIC
            ${sl_INCLUDE_DIRS}
        )
    endif()
endfunction() # pxr_static_library

function(pxr_plugin PLUGIN_NAME)
    set(options
        KATANA_PLUGIN
    )
    set(oneValueArgs 
        PREFIX 
    )
    set(multiValueArgs
        PUBLIC_CLASSES
        PUBLIC_HEADERS
        PRIVATE_CLASSES
        PRIVATE_HEADERS
        CPPFILES
        PYMODULE_CPPFILES
        PYTHON_FILES
        PYSIDE_UI_FILES
        LIBRARIES
        INCLUDE_DIRS
        RESOURCE_FILES
    )

    cmake_parse_arguments(sl
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    _classes(${PLUGIN_NAME} ${sl_PRIVATE_CLASSES} PRIVATE)
    _classes(${PLUGIN_NAME} ${sl_PUBLIC_CLASSES} PUBLIC)

    add_library(${PLUGIN_NAME}
        SHARED
        ${sl_CPPFILES} ${${PLUGIN_NAME}_CPPFILES}
        ${sl_PUBLIC_HEADERS} ${${PLUGIN_NAME}_PUBLIC_HEADERS}
        ${sl_PRIVATE_HEADERS} ${${PLUGIN_NAME}_PRIVATE_HEADERS}
    )

    if(sl_PYTHON_FILES)
        _install_python(${PLUGIN_NAME}
            FILES ${sl_PYTHON_FILES}
        )
    endif()

    # Plugins do not have a lib* prefix like usual shared libraries
    set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

    # MAYA plugins require .mll extension on Windows
    if(WIN32)
        if(${PLUGIN_NAME} STREQUAL "pxrUsd")
            set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX ".mll")
        endif()
    endif()

    if (PXR_INSTALL_SUBDIR)
        set(PLUGIN_INSTALL_PREFIX "${PXR_INSTALL_SUBDIR}/plugin")
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/${PXR_INSTALL_SUBDIR}/include/${PXR_PREFIX}/${PLUGIN_NAME}")
    else()
        set(PLUGIN_INSTALL_PREFIX "plugin")
        set(HEADER_INSTALL_PREFIX 
            "${CMAKE_INSTALL_PREFIX}/include/${PXR_PREFIX}/${PLUGIN_NAME}")
    endif()

    # Katana plugins install into a specific sub directory structure. Shared
    # objects, for example, install into plugin/Libs
    if (sl_KATANA_PLUGIN)
        set(PLUGIN_INSTALL_PREFIX ${PLUGIN_INSTALL_PREFIX}/Libs)

        # Ensure the katana plugin can pick up the top-level libs and the
        # top-level katana/libs
        set(rpath ${CMAKE_INSTALL_RPATH})
        set(rpath "$ORIGIN/../../lib:$ORIGIN/../../../../lib:${rpath}")

        set_target_properties(${PLUGIN_NAME} 
            PROPERTIES 
                INSTALL_RPATH ${rpath}
        )
    else()
        # Ensure this plugin can find the libs for its matching component, e.g.
        # maya/plugin/px_usdIO.so can find maya/lib/*.so
        set_target_properties(${PLUGIN_NAME}
            PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib"
        )
    endif()

    set_target_properties(${PLUGIN_NAME}
        PROPERTIES COMPILE_DEFINITIONS 
            "MFB_PACKAGE_NAME=${PXR_PACKAGE};MFB_ALT_PACKAGE_NAME=${PXR_PACKAGE}"
    )

    # Always bake the rpath.
    set_target_properties(${PLUGIN_NAME}
        PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    _install_headers(${PLUGIN_NAME}
        FILES
            ${sl_PUBLIC_HEADERS}
            ${sl_PRIVATE_HEADERS}
            ${${PLUGIN_NAME}_PUBLIC_HEADERS}
            ${${PLUGIN_NAME}_PRIVATE_HEADERS}
        PREFIX ${PXR_PREFIX}
    )

    set_target_properties(${PLUGIN_NAME}
        PROPERTIES
            PUBLIC_HEADER
                "${sl_PUBLIC_HEADERS};${${PLUGIN_NAME}_PUBLIC_HEADERS}"
            INTERFACE_INCLUDE_DIRECTORIES ""
    )

    # Install and include headers from the build directory.
    get_filename_component(
        PRIVATE_INC_DIR
        "${CMAKE_BINARY_DIR}/include"
        ABSOLUTE
    )
    target_include_directories(${PLUGIN_NAME}
        PRIVATE ${PRIVATE_INC_DIR}
    )

    # Allow #include'ing of headers within the same install subdir.
    if (PXR_INSTALL_SUBDIR)
        get_filename_component(
            SUBDIR_INC_DIR
            "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}/include"
            ABSOLUTE
        )

        target_include_directories(${PLUGIN_NAME}
            PRIVATE ${SUBDIR_INC_DIR}
        )
    endif()

    install(TARGETS ${PLUGIN_NAME}
        EXPORT pxrTargets
        LIBRARY DESTINATION ${PLUGIN_INSTALL_PREFIX}
        ARCHIVE DESTINATION ${PLUGIN_INSTALL_PREFIX}
        RUNTIME DESTINATION ${PLUGIN_INSTALL_PREFIX}
        PUBLIC_HEADER DESTINATION ${HEADER_INSTALL_PREFIX}
    )

    export(TARGETS ${PLUGIN_NAME}
        APPEND
        FILE "${PROJECT_BINARY_DIR}/pxrTargets.cmake"
    )

    if (PXR_MALLOC_LIBRARY)
        target_link_libraries(${PLUGIN_NAME}
            ${sl_LIBRARIES}
        )
    else()
         target_link_libraries(${PLUGIN_NAME}
            ${sl_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
        )
    endif()

    # Include system headers before our own.  We define several headers
    # that conflict; for example, half.h in EXR versus gf
    if (sl_INCLUDE_DIRS)
        target_include_directories(${PLUGIN_NAME}
            BEFORE
            PUBLIC
            ${sl_INCLUDE_DIRS}
        )
    endif()

    if (sl_RESOURCE_FILES)
        _get_install_dir(plugin PLUGINS_PREFIX)
        set(LIBRARY_NAME ${PLUGIN_NAME})

        _install_resource_files(${sl_RESOURCE_FILES})
    endif()

    if (sl_PYSIDE_UI_FILES)
        _get_install_dir(plugin PLUGINS_PREFIX)
        set(LIBRARY_NAME ${PLUGIN_NAME})

        _install_pyside_ui_files(${sl_PYSIDE_UI_FILES})
    endif()        

    # Build python module.
    if(DEFINED sl_PYMODULE_CPPFILES)
        pxr_shared_library(
            "_${PLUGIN_NAME}"
            PYTHON_LIBRARY
            CPPFILES ${sl_PYMODULE_CPPFILES}
            LIBRARIES ${PLUGIN_NAME}
        )
    endif()
endfunction() # pxr_plugin

function(pxr_setup_python)
    get_property(pxrPythonModules GLOBAL PROPERTY PXR_PYTHON_MODULES)

    # A new list where each python module is quoted
    set(converted "")
    foreach(module ${pxrPythonModules})
        list(APPEND converted "'${module}'")
    endforeach()

    # Join these with a ', '
    string(REPLACE ";" ", " pyModulesStr "${converted}")

    # Install a pxr __init__.py with an appropriate __all__
    _get_install_dir(lib/python/pxr installPrefix)
    install(CODE
        "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${installPrefix}/__init__.py\" \"__all__ = [${pyModulesStr}]\n\")"
    )
endfunction() # pxr_setup_python

function (pxr_create_test_module MODULE_NAME)
    cmake_parse_arguments(tm "" "INSTALL_PREFIX;SOURCE_DIR" "" ${ARGN})

    if (NOT tm_SOURCE_DIR)
        set(tm_SOURCE_DIR testenv)
    endif()

    # Look specifically for an __init__.py and a plugInfo.json prefixed by the
    # module name. These will be installed without the module prefix.
    set(initPyFile ${tm_SOURCE_DIR}/${MODULE_NAME}__init__.py)
    set(plugInfoFile ${tm_SOURCE_DIR}/${MODULE_NAME}_plugInfo.json)

    if (EXISTS ${initPyFile})
        install(
            FILES 
                ${initPyFile}
            RENAME 
                __init__.py
            DESTINATION 
                tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
        )
    endif()

    if (EXISTS ${plugInfoFile})
        install(
            FILES 
                ${plugInfoFile}
            RENAME 
                plugInfo.json
            DESTINATION 
                tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
        )
    endif()
endfunction() # pxr_create_test_module

function(pxr_build_test_shared_lib LIBRARY_NAME)
    cmake_parse_arguments(bt
        "" ""
        "LIBRARIES;CPPFILES"
        ${ARGN}
    )
    
    add_library(${LIBRARY_NAME}
        SHARED
        ${bt_CPPFILES}
        )
    target_link_libraries(${LIBRARY_NAME}
        ${bt_LIBRARIES}
    )
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES 
            INSTALL_RPATH_USE_LINK_PATH TRUE
            FOLDER "${PXR_PREFIX}/tests/lib"
    )

    # We always want this test to build after the package it's under, even if
    # it doesn't link directly. This ensures that this test is able to include
    # headers from its parent package.
    add_dependencies(${LIBRARY_NAME} ${PXR_PACKAGE})

    # Test libraries can include the private headers of their parent PXR_PACKAGE
    # library
    target_include_directories(${LIBRARY_NAME}
        PRIVATE $<TARGET_PROPERTY:${PXR_PACKAGE},INCLUDE_DIRECTORIES>
    )

    install(TARGETS ${LIBRARY_NAME}
		ARCHIVE DESTINATION "tests/lib"
        LIBRARY DESTINATION "tests/lib"
        ARCHIVE DESTINATION "tests/lib"
    )
endfunction() # pxr_build_test_shared_lib

function(pxr_build_test TEST_NAME)
    cmake_parse_arguments(bt
        "" ""
        "LIBRARIES;CPPFILES"
        ${ARGN}
    )

    add_executable(${TEST_NAME}
        ${bt_CPPFILES}
    )
    target_link_libraries(${TEST_NAME}
        ${bt_LIBRARIES}
    )
    target_include_directories(${TEST_NAME}
        PRIVATE $<TARGET_PROPERTY:${PXR_PACKAGE},INCLUDE_DIRECTORIES>
    )
    set_target_properties(${TEST_NAME}
        PROPERTIES 
            INSTALL_RPATH_USE_LINK_PATH TRUE
            POSITION_INDEPENDENT_CODE ON
            FOLDER "${PXR_PREFIX}/tests/bin"
    )

    install(TARGETS ${TEST_NAME}
        RUNTIME DESTINATION "tests"
    )
endfunction() # pxr_build_test

function(pxr_test_scripts)
    foreach(file ${ARGN})
        get_filename_component(destFile ${file} NAME_WE)
        install(
            PROGRAMS ${file}
            DESTINATION tests
            RENAME ${destFile}
        )
    endforeach()
endfunction() # pxr_test_scripts

function(pxr_install_test_dir)
    cmake_parse_arguments(bt
        ""
        "SRC;DEST"
        ""
        ${ARGN}
    )

    install(
        DIRECTORY ${bt_SRC}/
        DESTINATION tests/ctest/${bt_DEST}
    )
endfunction() # pxr_install_test_dir

function(pxr_register_test TEST_NAME)
    cmake_parse_arguments(bt
        "PYTHON"
        "COMMAND;STDOUT_REDIRECT;STDERR_REDIRECT;DIFF_COMPARE;EXPECTED_RETURN_CODE;TESTENV"
        "ENV"
        ${ARGN}
    )

    # This harness is a filter which allows us to manipulate the test run,
    # e.g. by changing the environment, changing the expected return code, etc.
    set(testWrapperCmd ${PROJECT_SOURCE_DIR}/cmake/macros/testWrapper.py --verbose)

    if (bt_STDOUT_REDIRECT)
        set(testWrapperCmd ${testWrapperCmd} --stdout-redirect=${bt_STDOUT_REDIRECT})
    endif()

    if (bt_STDERR_REDIRECT)
        set(testWrapperCmd ${testWrapperCmd} --stderr-redirect=${bt_STDERR_REDIRECT})
    endif()

    # Not all tests will have testenvs, but if they do let the wrapper know so
    # it can copy the testenv contents into the run directory. By default,
    # assume the testenv has the same name as the test but allow it to be
    # overridden by specifying TESTENV.
    if (bt_TESTENV)
        set(testenvDir ${CMAKE_INSTALL_PREFIX}/tests/ctest/${bt_TESTENV})
    else()
        set(testenvDir ${CMAKE_INSTALL_PREFIX}/tests/ctest/${TEST_NAME})
    endif()

    set(testWrapperCmd ${testWrapperCmd} --testenv-dir=${testenvDir})

    if (bt_DIFF_COMPARE)
        set(testWrapperCmd ${testWrapperCmd} --diff-compare=${bt_DIFF_COMPARE})

        # For now the baseline directory is assumed by convention from the test
        # name. There may eventually be cases where we'd want to specify it by
        # an argument though.
        set(baselineDir ${testenvDir}/baseline)
        set(testWrapperCmd ${testWrapperCmd} --baseline-dir=${baselineDir})
    endif()

    if (bt_EXPECTED_RETURN_CODE)
        set(testWrapperCmd ${testWrapperCmd}
            --expected-return-code=${bt_EXPECTED_RETURN_CODE})
    endif()

    if (bt_ENV)
        foreach(env ${bt_ENV})
            set(testWrapperCmd ${testWrapperCmd} --env-var=${env})
        endforeach()
    endif()

    # Ensure that Python imports the Python files built by this build
    set(testWrapperCmd ${testWrapperCmd}
        --env-var=PYTHONPATH=${CMAKE_INSTALL_PREFIX}/lib/python:${PYTHON_PATH})

    # Ensure we run with the python executable known to the build
    if (bt_PYTHON)
        set(testCmd "${PYTHON_EXECUTABLE} ${bt_COMMAND}")
    else()
        set(testCmd "${bt_COMMAND}")
    endif()

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${PYTHON_EXECUTABLE} ${testWrapperCmd} ${testCmd}
    )
endfunction() # pxr_register_test

function(pxr_setup_plugins)
    _get_share_install_dir(SHARE_INSTALL_PREFIX)

    # Install a top-level plugInfo.json in the shared area
    install(CODE
        "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${SHARE_INSTALL_PREFIX}/plugins/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
    )
endfunction() # pxr_setup_plugins

function(pxr_katana_nodetypes NODE_TYPES)
    set(installDir ${PXR_INSTALL_SUBDIR}/plugin/Plugins/NodeTypes)

    set(pyFiles "")
    set(importLines "")

    foreach (nodeType ${NODE_TYPES})
        list(APPEND pyFiles ${nodeType}.py)
        set(importLines "import ${nodeType}\n")
    endforeach()

    install(PROGRAMS
        ${pyFiles}
        DESTINATION ${installDir}
    )

    # Install a __init__.py that imports all the known node types
    install(CODE
        "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${installDir}/__init__.py\" \"${importLines}\")"
    )
endfunction() # pxr_katana_nodetypes

