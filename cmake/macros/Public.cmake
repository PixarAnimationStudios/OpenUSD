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

function(pxr_python_bin BIN_NAME)
    set(oneValueArgs
        PYTHON_FILE
    )
    set(multiValueArgs
        DEPENDENCIES
    )
    cmake_parse_arguments(pb
        ""
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # If we can't build Python modules then do nothing.
    if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
        message(STATUS "Skipping Python program ${BIN_NAME}, Python modules required")
        return()
    endif()

    _get_install_dir(bin installDir)

    # Source file.
    if( "${pb_PYTHON_FILE}" STREQUAL "")
        set(infile ${CMAKE_CURRENT_SOURCE_DIR}/${BIN_NAME}.py)
    else()
        set(infile ${CMAKE_CURRENT_SOURCE_DIR}/${pb_PYTHON_FILE})
    endif()

    # Destination file.
    set(outfile ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME})

    # /pxrpythonsubst will be replaced with the full path to the configured
    # python executable. This doesn't use the CMake ${...} or @...@ syntax
    # for backwards compatibility with other build systems.
    add_custom_command(
        OUTPUT ${outfile}
        DEPENDS ${infile}
        COMMENT "Substituting Python shebang"
        COMMAND
            ${PYTHON_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/cmake/macros/shebang.py
            ${PXR_PYTHON_SHEBANG}
            ${infile}
            ${outfile}
    )
    list(APPEND outputs ${outfile})

    install(
        PROGRAMS ${outfile}
        DESTINATION ${installDir}
        RENAME ${BIN_NAME}
    )

    # Windows by default cannot execute Python files so here
    # we create a batch file wrapper that invokes the python
    # files.
    if(WIN32)
        add_custom_command(
            OUTPUT ${outfile}.cmd
            COMMENT "Creating Python cmd wrapper"
            COMMAND
                ${PYTHON_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/cmake/macros/shebang.py
                ${BIN_NAME}
                ${outfile}.cmd
        )
        list(APPEND outputs ${outfile}.cmd)

        install(
            PROGRAMS ${outfile}.cmd
            DESTINATION ${installDir}
            RENAME ${BIN_NAME}.cmd
        )
    endif()

    # Make sure the custom commands are executed by the default rule.
    add_custom_target(${BIN_NAME} ALL
        DEPENDS ${outputs} ${pb_DEPENDENCIES}
    )

    _get_folder("" folder)
    set_target_properties(${BIN_NAME}
        PROPERTIES
            FOLDER "${folder}"
    )
endfunction() # pxr_python_bin

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

    # Turn PIC ON otherwise ArchGetAddressInfo() on Linux may yield
    # unexpected results.
    _get_folder("" folder)
    set_target_properties(${BIN_NAME}
        PROPERTIES
            FOLDER "${folder}"
            POSITION_INDEPENDENT_CODE ON
    )

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

    _pxr_init_rpath(rpath "${installDir}")
    _pxr_install_rpath(rpath ${BIN_NAME})

    _pxr_target_link_libraries(${BIN_NAME}
        ${cb_LIBRARIES}
        ${PXR_MALLOC_LIBRARY}
    )

    install(TARGETS 
        ${BIN_NAME}
        DESTINATION ${installDir}
    )
endfunction()

function(pxr_library NAME)
    set(options
        DISABLE_PRECOMPILED_HEADERS
        KATANA_PLUGIN
        MAYA_PLUGIN
    )
    set(oneValueArgs
        TYPE
        PRECOMPILED_HEADER_NAME
    )
    set(multiValueArgs
        PUBLIC_CLASSES
        PUBLIC_HEADERS
        PRIVATE_CLASSES
        PRIVATE_HEADERS
        CPPFILES
        LIBRARIES
        INCLUDE_DIRS
        RESOURCE_FILES
        PYMODULE_CPPFILES
        PYTHON_FILES
        PYSIDE_UI_FILES
    )

    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Collect libraries.
    get_property(help CACHE PXR_ALL_LIBS PROPERTY HELPSTRING)
    list(APPEND PXR_ALL_LIBS ${NAME})
    set(PXR_ALL_LIBS "${PXR_ALL_LIBS}" CACHE INTERNAL "${help}")
    if(args_TYPE STREQUAL "STATIC")
        # Note if this library is explicitly STATIC.
        get_property(help CACHE PXR_STATIC_LIBS PROPERTY HELPSTRING)
        list(APPEND PXR_STATIC_LIBS ${NAME})
        set(PXR_STATIC_LIBS "${PXR_STATIC_LIBS}" CACHE INTERNAL "${help}")
    endif()

    # Expand classes into filenames.
    _classes(${NAME} ${args_PRIVATE_CLASSES} PRIVATE)
    _classes(${NAME} ${args_PUBLIC_CLASSES} PUBLIC)

    # Custom tweaks.
    if(args_TYPE STREQUAL "PLUGIN")
        # We can't build plugins if we're not building shared libraries.
        if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
            message(STATUS "Skipping plugin ${NAME}, shared libraries required")
            return()
        endif()

        set(prefix "")
        set(suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})

        # Katana plugins install into a specific sub directory structure.
        # In particular, shared objects install into plugin/Libs
        if(args_KATANA_PLUGIN)
            set(subdir "Libs")
        endif()

        # Maya plugins require the .mll suffix on Windows and .bundle on OSX.
        if(args_MAYA_PLUGIN)
            if (WIN32)
                set(suffix ".mll")
            elseif(APPLE)
                set(suffix ".bundle")
            endif()
        endif()
    else()
        # If the caller didn't specify the library type then choose the
        # type now.
        if("x${args_TYPE}" STREQUAL "x")
            if(PXR_BUILD_MONOLITHIC OR NOT BUILD_SHARED_LIBS)
                # We build static libraries when building monolithic since
                # the last step is to link all of the static libraries into
                # the monolithic library.
                set(args_TYPE "STATIC")
            else()
                set(args_TYPE "SHARED")
            endif()
        endif()

        set(prefix "${PXR_LIB_PREFIX}")
        if(args_TYPE STREQUAL "STATIC")
            set(suffix ${CMAKE_STATIC_LIBRARY_SUFFIX})
        else()
            set(suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
        endif()
    endif()

    set(pch "ON")
    if(args_DISABLE_PRECOMPILED_HEADERS)
        set(pch "OFF")
    endif()

    _pxr_library(${NAME}
        TYPE "${args_TYPE}"
        PREFIX "${prefix}"
        SUFFIX "${suffix}"
        SUBDIR "${subdir}"
        CPPFILES "${args_CPPFILES};${${NAME}_CPPFILES}"
        PUBLIC_HEADERS "${args_PUBLIC_HEADERS};${${NAME}_PUBLIC_HEADERS}"
        PRIVATE_HEADERS "${args_PRIVATE_HEADERS};${${NAME}_PRIVATE_HEADERS}"
        LIBRARIES "${args_LIBRARIES}"
        INCLUDE_DIRS "${args_INCLUDE_DIRS}"
        RESOURCE_FILES "${args_RESOURCE_FILES}"
        PRECOMPILED_HEADERS "${pch}"
        PRECOMPILED_HEADER_NAME "${args_PRECOMPILED_HEADER_NAME}"
        LIB_INSTALL_PREFIX_RESULT libInstallPrefix
    )

    if(args_PYMODULE_CPPFILES OR args_PYTHON_FILES OR args_PYSIDE_UI_FILES)
        _pxr_python_module(
            ${NAME}
            WRAPPED_LIB_INSTALL_PREFIX "${libInstallPrefix}"
            PYTHON_FILES ${args_PYTHON_FILES}
            PYSIDE_UI_FILES ${args_PYSIDE_UI_FILES}
            CPPFILES ${args_PYMODULE_CPPFILES}
            INCLUDE_DIRS ${args_INCLUDE_DIRS}
            PRECOMPILED_HEADERS ${pch}
            PRECOMPILED_HEADER_NAME ${args_PRECOMPILED_HEADER_NAME}
        )
    endif()
endfunction()

macro(pxr_shared_library NAME)
    pxr_library(${NAME} TYPE "SHARED" ${ARGN})
endmacro(pxr_shared_library)

macro(pxr_static_library NAME)
    pxr_library(${NAME} TYPE "STATIC" ${ARGN})
endmacro(pxr_static_library)

macro(pxr_plugin NAME)
    pxr_library(${NAME} TYPE "PLUGIN" ${ARGN})
endmacro(pxr_plugin)

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

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated_modules_init.py"
         "__all__ = [${pyModulesStr}]\n")

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated_modules_init.py"
            DESTINATION ${installPrefix}
            RENAME "__init__.py")
endfunction() # pxr_setup_python

function (pxr_create_test_module MODULE_NAME)
    # If we can't build Python modules then do nothing.
    if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
        return()
    endif()

    if (PXR_BUILD_TESTS) 
        cmake_parse_arguments(tm "" "INSTALL_PREFIX;SOURCE_DIR" "" ${ARGN})

        if (NOT tm_SOURCE_DIR)
            set(tm_SOURCE_DIR testenv)
        endif()

        # Look specifically for an __init__.py and a plugInfo.json prefixed by the
        # module name. These will be installed without the module prefix.
        set(initPyFile ${tm_SOURCE_DIR}/${MODULE_NAME}__init__.py)
        set(plugInfoFile ${tm_SOURCE_DIR}/${MODULE_NAME}_plugInfo.json)

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${initPyFile}")
            install(
                FILES 
                    ${initPyFile}
                RENAME 
                    __init__.py
                DESTINATION 
                    tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
            )
        endif()

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${plugInfoFile}")
            install(
                FILES 
                    ${plugInfoFile}
                RENAME 
                    plugInfo.json
                DESTINATION 
                    tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
            )
        endif()
    endif()
endfunction() # pxr_create_test_module

function(pxr_build_test_shared_lib LIBRARY_NAME)
    if (PXR_BUILD_TESTS)
        cmake_parse_arguments(bt
            ""
            "INSTALL_PREFIX;SOURCE_DIR"
            "LIBRARIES;CPPFILES"
            ${ARGN}
        )
        
        add_library(${LIBRARY_NAME}
            SHARED
            ${bt_CPPFILES}
        )
        _pxr_target_link_libraries(${LIBRARY_NAME}
            ${bt_LIBRARIES}
        )
        _get_folder("tests/lib" folder)
        set_target_properties(${LIBRARY_NAME}
            PROPERTIES 
                FOLDER "${folder}"
        )

        # Find libraries under the install prefix, which has the core USD
        # libraries.
        _pxr_init_rpath(rpath "tests/lib")
        _pxr_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
        _pxr_install_rpath(rpath ${LIBRARY_NAME})

        if (NOT bt_SOURCE_DIR)
            set(bt_SOURCE_DIR testenv)
        endif()
        set(testPlugInfoSrcPath ${bt_SOURCE_DIR}/${LIBRARY_NAME}_plugInfo.json)

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${testPlugInfoSrcPath}")
            set(TEST_PLUG_INFO_RESOURCE_PATH "Resources")
            set(TEST_PLUG_INFO_ROOT "..")
            set(LIBRARY_FILE "${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

            set(testPlugInfoLibDir "tests/${bt_INSTALL_PREFIX}/lib/${LIBRARY_NAME}")
            set(testPlugInfoResourceDir "${testPlugInfoLibDir}/${TEST_PLUG_INFO_RESOURCE_PATH}")
            set(testPlugInfoPath "${CMAKE_BINARY_DIR}/${testPlugInfoResourceDir}/plugInfo.json")

            file(RELATIVE_PATH 
                TEST_PLUG_INFO_LIBRARY_PATH
                "${CMAKE_INSTALL_PREFIX}/${testPlugInfoLibDir}"
                "${CMAKE_INSTALL_PREFIX}/tests/lib/${LIBRARY_FILE}")

            configure_file("${testPlugInfoSrcPath}" "${testPlugInfoPath}")
            install(
                FILES ${testPlugInfoPath}
                DESTINATION ${testPlugInfoResourceDir})
        endif()

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
            LIBRARY DESTINATION "tests/lib"
            ARCHIVE DESTINATION "tests/lib"
            RUNTIME DESTINATION "tests/lib"
        )
    endif()
endfunction() # pxr_build_test_shared_lib

function(pxr_build_test TEST_NAME)
    if (PXR_BUILD_TESTS)
        cmake_parse_arguments(bt
            "" ""
            "LIBRARIES;CPPFILES"
            ${ARGN}
        )

        add_executable(${TEST_NAME}
            ${bt_CPPFILES}
        )
        # Turn PIC ON otherwise ArchGetAddressInfo() on Linux may yield
        # unexpected results.
        _get_folder("tests/bin" folder)
        set_target_properties(${TEST_NAME}
            PROPERTIES 
                FOLDER "${folder}"
            	POSITION_INDEPENDENT_CODE ON
        )
        target_include_directories(${TEST_NAME}
            PRIVATE $<TARGET_PROPERTY:${PXR_PACKAGE},INCLUDE_DIRECTORIES>
        )
        _pxr_target_link_libraries(${TEST_NAME}
            ${bt_LIBRARIES}
        )

        # Find libraries under the install prefix, which has the core USD
        # libraries.
        _pxr_init_rpath(rpath "tests")
        _pxr_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
        _pxr_install_rpath(rpath ${TEST_NAME})

        install(TARGETS ${TEST_NAME}
            RUNTIME DESTINATION "tests"
        )
    endif()
endfunction() # pxr_build_test

function(pxr_test_scripts)
    # If we can't build Python modules then do nothing.
    if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
        return()
    endif()

    if (PXR_BUILD_TESTS)
        foreach(file ${ARGN})
            get_filename_component(destFile ${file} NAME_WE)
            install(
                PROGRAMS ${file}
                DESTINATION tests
                RENAME ${destFile}
            )
        endforeach()
    endif()
endfunction() # pxr_test_scripts

function(pxr_install_test_dir)
    if (PXR_BUILD_TESTS)
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
    endif()
endfunction() # pxr_install_test_dir

function(pxr_register_test TEST_NAME)
    if (PXR_BUILD_TESTS)
        cmake_parse_arguments(bt
            "PYTHON;REQUIRES_DISPLAY;REQUIRES_SHARED_LIBS;REQUIRES_PYTHON_MODULES" 
            "CUSTOM_PYTHON;COMMAND;STDOUT_REDIRECT;STDERR_REDIRECT;DIFF_COMPARE;POST_COMMAND;POST_COMMAND_STDOUT_REDIRECT;POST_COMMAND_STDERR_REDIRECT;PRE_COMMAND;PRE_COMMAND_STDOUT_REDIRECT;PRE_COMMAND_STDERR_REDIRECT;FILES_EXIST;FILES_DONT_EXIST;CLEAN_OUTPUT;EXPECTED_RETURN_CODE;TESTENV"
            "ENV;PRE_PATH;POST_PATH"
            ${ARGN}
        )

        # Discard tests that required shared libraries.
        if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
            # Explicit requirement.  This is for C++ tests that dynamically
            # load libraries linked against USD code.  These tests will have
            # multiple copies of symbols and will likely re-execute
            # ARCH_CONSTRUCTOR and registration functions, which will almost
            # certainly cause problems.
            if(bt_REQUIRES_SHARED_LIBS)
                message(STATUS "Skipping test ${TEST_NAME}, shared libraries required")
                return()
            endif()

            # Implicit requirement.  Python modules require shared USD
            # libraries.  If the test runs python it's certainly going
            # to load USD modules.  If the test uses C++ to load USD
            # modules it tells us via REQUIRES_PYTHON_MODULES.
            if(bt_PYTHON OR bt_CUSTOM_PYTHON OR bt_REQUIRES_PYTHON_MODULES)
                message(STATUS "Skipping test ${TEST_NAME}, Python modules required")
                return()
            endif()
        endif()

        # This harness is a filter which allows us to manipulate the test run, 
        # e.g. by changing the environment, changing the expected return code, etc.
        set(testWrapperCmd ${PROJECT_SOURCE_DIR}/cmake/macros/testWrapper.py --verbose)

        if (bt_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --stdout-redirect=${bt_STDOUT_REDIRECT})
        endif()

        if (bt_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --stderr-redirect=${bt_STDERR_REDIRECT})
        endif()

        if (bt_REQUIRES_DISPLAY)
            set(testWrapperCmd ${testWrapperCmd} --requires-display)
        endif()

        if (bt_PRE_COMMAND_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --pre-command-stdout-redirect=${bt_PRE_COMMAND_STDOUT_REDIRECT})
        endif()

        if (bt_PRE_COMMAND_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --pre-command-stderr-redirect=${bt_PRE_COMMAND_STDERR_REDIRECT})
        endif()

        if (bt_POST_COMMAND_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --post-command-stdout-redirect=${bt_POST_COMMAND_STDOUT_REDIRECT})
        endif()

        if (bt_POST_COMMAND_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --post-command-stderr-redirect=${bt_POST_COMMAND_STDERR_REDIRECT})
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

        if (bt_CLEAN_OUTPUT)
            set(testWrapperCmd ${testWrapperCmd} --clean-output-paths=${bt_CLEAN_OUTPUT})
        endif()

        if (bt_FILES_EXIST)
            set(testWrapperCmd ${testWrapperCmd} --files-exist=${bt_FILES_EXIST})
        endif()

        if (bt_FILES_DONT_EXIST)
            set(testWrapperCmd ${testWrapperCmd} --files-dont-exist=${bt_FILES_DONT_EXIST})
        endif()

        if (bt_PRE_COMMAND)
            set(testWrapperCmd ${testWrapperCmd} --pre-command=${bt_PRE_COMMAND})
        endif()

        if (bt_POST_COMMAND)
            set(testWrapperCmd ${testWrapperCmd} --post-command=${bt_POST_COMMAND})
        endif()

        if (bt_EXPECTED_RETURN_CODE)
            set(testWrapperCmd ${testWrapperCmd} --expected-return-code=${bt_EXPECTED_RETURN_CODE})
        endif()

        if (bt_ENV)
            foreach(env ${bt_ENV})
                set(testWrapperCmd ${testWrapperCmd} --env-var=${env})
            endforeach()
        endif()

        if (bt_PRE_PATH)
            foreach(path ${bt_PRE_PATH})
                set(testWrapperCmd ${testWrapperCmd} --pre-path=${path})
            endforeach()
        endif()

        if (bt_POST_PATH)
            foreach(path ${bt_POST_PATH})
                set(testWrapperCmd ${testWrapperCmd} --post-path=${path})
            endforeach()
        endif()

        # Ensure that Python imports the Python files built by this build.
        # On Windows convert backslash to slash and don't change semicolons
        # to colons.
        set(_testPythonPath "${CMAKE_INSTALL_PREFIX}/lib/python;$ENV{PYTHONPATH}")
        if(WIN32)
            string(REGEX REPLACE "\\\\" "/" _testPythonPath "${_testPythonPath}")
        else()
            string(REPLACE ";" ":" _testPythonPath "${_testPythonPath}")
        endif()

        # Ensure we run with the appropriate python executable.
        if (bt_CUSTOM_PYTHON)
            set(testCmd "${bt_CUSTOM_PYTHON} ${bt_COMMAND}")
        elseif (bt_PYTHON)
            set(testCmd "${PYTHON_EXECUTABLE} ${bt_COMMAND}")
        else()
            set(testCmd "${bt_COMMAND}")
        endif()

        add_test(
            NAME ${TEST_NAME}
            COMMAND ${PYTHON_EXECUTABLE} ${testWrapperCmd}
                    "--env-var=PYTHONPATH=${_testPythonPath}" ${testCmd}
        )
    endif()
endfunction() # pxr_register_test

function(pxr_setup_plugins)
    set(SHARE_INSTALL_PREFIX "share/usd")

    # Install a top-level plugInfo.json in the shared area and into the 
    # top-level plugin area
    _get_resources_dir_name(resourcesDir)
    set(plugInfoContents "{\n    \"Includes\": [ \"*/${resourcesDir}/\" ]\n}\n")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugins_plugInfo.json"
         "${plugInfoContents}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugins_plugInfo.json"
            DESTINATION "${SHARE_INSTALL_PREFIX}/plugins"
            RENAME "plugInfo.json")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/usd_plugInfo.json"
         "${plugInfoContents}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/usd_plugInfo.json"
            DESTINATION plugin/usd
            RENAME "plugInfo.json")
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
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated_NodeTypes_init.py"
         "${importLines}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated_NodeTypes_init.py"
            DESTINATION "${installDir}"
            RENAME "__init__.py")
endfunction() # pxr_katana_nodetypes
