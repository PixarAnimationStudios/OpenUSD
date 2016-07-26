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
include(Version)

function(_install_headers LIBRARY_NAME)
    set(options  "")
    set(oneValueArgs PREFIX)
    set(multiValueArgs FILES)
    cmake_parse_arguments(_install_headers
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    
    if (PXR_INSTALL_SUBDIR)
        set(installPrefix "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}") 
    else()
        set(installPrefix "${CMAKE_BINARY_DIR}")
    endif()

    set(hpath "${_install_headers_PREFIX}/${LIBRARY_NAME}")
    set(header_dest_dir "${installPrefix}/include/${hpath}")
    if( NOT "${_install_headers_FILES}" STREQUAL "")
        set(files_copied "")
        foreach (f ${_install_headers_FILES})
            set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
            set(outfile "${header_dest_dir}/${f}")
            list(APPEND files_copied ${outfile})
            add_custom_command(
                OUTPUT ${outfile}
                COMMAND "${CMAKE_COMMAND}"
                ARGS -E copy "${infile}" "${outfile}"
                MAIN_DEPENDENCY "${infile}"
                COMMENT "Copying ${f} ..."
                VERBATIM
        )
        endforeach()
    endif()
endfunction() # _install_headers

# Converts a library name, such as _tf.so to the internal module name given
# our naming conventions, e.g. Tf
function(_get_python_module_name LIBRARY_FILENAME MODULE_NAME)
    # Library names are either something like tf.so for shared libraries
    # or _tf.so for Python module libraries. We want to strip the leading
    # "_" off.
    string(REPLACE "_" "" LIBNAME ${LIBRARY_FILENAME})
    string(SUBSTRING ${LIBNAME} 0 1 LIBNAME_FL)
    string(TOUPPER ${LIBNAME_FL} LIBNAME_FL)
    string(SUBSTRING ${LIBNAME} 1 -1 LIBNAME_SUFFIX)
    set(${MODULE_NAME}
        "${LIBNAME_FL}${LIBNAME_SUFFIX}"
        PARENT_SCOPE
    )
endfunction() # _get_python_module_name

function(_plugInfo_subst libName plugInfoPath)
    # Generate plugInfo.json files from a template. Note that we can't use
    # the $<TARGET_FILE_NAME:tgt> generator expression here because 
    # configure_file will run at configure time while the generators will only
    # run after.
    set(libFile ${CMAKE_SHARED_LIBRARY_PREFIX}${libName}${CMAKE_SHARED_LIBRARY_SUFFIX})

    # The root resource directory is in $PREFIX/share/usd/$LIB/resource but the 
    # libs are actually in $PREFIX/lib. The lib path can then be specified
    # relatively as below.
    set(PLUG_INFO_LIBRARY_PATH "../../../../lib/${libFile}")
    set(PLUG_INFO_RESOURCE_PATH "resources")
    set(PLUG_INFO_PLUGIN_NAME "pxr.${libName}")
    set(PLUG_INFO_ROOT "..")

    configure_file(
        ${plugInfoPath}
        ${CMAKE_CURRENT_BINARY_DIR}/${plugInfoPath}
    )
endfunction() # _plugInfo_subst

# Install compiled python files alongside the python object,
# e.g. lib/python/pxr/Ar/__init__.pyc
function(_install_python LIBRARY_NAME)
    set(options  "")
    set(oneValueArgs "")
    set(multiValueArgs FILES)
    cmake_parse_arguments(ip
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    set(libPythonPrefix lib/python)

    string(SUBSTRING ${LIBRARY_NAME} 0 1 LIBNAME_FL)
    string(TOUPPER ${LIBNAME_FL} LIBNAME_FL)
    string(SUBSTRING ${LIBRARY_NAME} 1 -1 LIBNAME_SUFFIX)
    set(LIBRARY_INSTALLNAME "${LIBNAME_FL}${LIBNAME_SUFFIX}")

    foreach(file ${ip_FILES})
        set(filesToInstall "")
        set(installDest 
            "${libPythonPrefix}/pxr/${LIBRARY_INSTALLNAME}")

        # Only attempt to compile .py files. Files like plugInfo.json may also
        # be in this list
        if (${file} MATCHES ".py$")
            get_filename_component(file_we ${file} NAME_WE)

            # Preserve any directory prefix, just strip the extension. This
            # directory needs to exist in the binary dir for the COMMAND below
            # to work.
            get_filename_component(dir ${file} PATH)
            if (dir)
                file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir})
                set(file_we ${dir}/${file_we})
                set(installDest ${installDest}/${dir})
            endif()

            set(outfile ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.pyc)
            list(APPEND files_copied ${outfile})
            add_custom_command(OUTPUT ${outfile}
                COMMAND
                    ${PYTHON_EXECUTABLE}
                    ${PROJECT_SOURCE_DIR}/cmake/macros/compilePython.py
                    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
                    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
                    ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.pyc
            )
            list(APPEND filesToInstall ${CMAKE_CURRENT_SOURCE_DIR}/${file})
            list(APPEND filesToInstall ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.pyc)
        elseif (${file} STREQUAL "plugInfo.json")
            _plugInfo_subst(${LIBRARY_NAME} ${file})
            list(APPEND filesToInstall ${CMAKE_CURRENT_BINARY_DIR}/${file})
        else()
            list(APPEND filesToInstall ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endif()

        # Note that we always install under lib/python/pxr, even if we are in 
        # the third_party project. This means the import will always look like
        # 'from pxr import X'. We need to do this per-loop iteration because
        # the installDest may be different due to the presence of subdirs.
        INSTALL(
            FILES
                ${filesToInstall}
            DESTINATION
                "${installDest}"
        )
    endforeach()

    add_custom_target(${LIBRARY_NAME}_pythonfiles ALL
        DEPENDS
        ${files_copied}
    )

    set_target_properties(${LIBRARY_NAME}_pythonfiles
        PROPERTIES
            FOLDER "${PXR_PREFIX}/_python"
    )
endfunction() #_install_python

function(_install_resource_files)
    set(resourceFiles "")
    foreach(resourceFile ${ARGN})
        # plugInfo.json go through an initial template substitution step files
        # install it from the binary (gen) directory specified by the full
        # path. Otherwise, use the original relative path which is relative to
        # the source directory.
        if (${resourceFile} STREQUAL "plugInfo.json")
            _plugInfo_subst(${LIBRARY_NAME} ${resourceFile})
            list(APPEND resourceFiles "${CMAKE_CURRENT_BINARY_DIR}/${resourceFile}")
        else()
            list(APPEND resourceFiles ${resourceFile})
        endif()
    endforeach()

    # Resource files install into a structure that looks like:
    # share/
    #     usd/
    #         ${LIBRARY_NAME}/
    #             resources/
    #                 resourceFileA
    #                 subdir/
    #                     resourceFileB
    #                     resourceFileC
    #                 ...
    #
    foreach(f ${resourceFiles})
        # Don't install subdirs for absolute paths, there's no way to tell
        # what the intended subdir structure is. In practice, any absolute paths
        # should only come from the plugInfo.json processing above, which 
        # install at the top-level anyway.
        if (NOT IS_ABSOLUTE ${f})
            get_filename_component(dirPath ${f} PATH)
        endif()

        install(
            FILES ${f}
            DESTINATION ${PLUGINS_PREFIX}/${LIBRARY_NAME}/resources/${dirPath}
        )
    endforeach()
endfunction() # _install_resource_files

function(_classes LIBRARY_NAME)
    # Install headers to build or install prefix
    set(options PUBLIC PRIVATE)
    cmake_parse_arguments(classes
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    # If both get set, fall back to public.
    if(${classes_PUBLIC})
        set(VISIBILITY "PUBLIC")
    elseif(${classes_PRIVATE})
        set(VISIBILITY "PRIVATE")
    else()
        message(FATAL_ERROR
            "Library ${LIBRARY_NAME} has implicit visibility.  "
            "Provide PUBLIC or PRIVATE to classes() call.")
    endif()

    # Should the classes have an argument name?
    foreach(cls ${classes_UNPARSED_ARGUMENTS})
        list(APPEND ${LIBRARY_NAME}_${VISIBILITY}_HEADERS ${cls}.h)
        list(APPEND ${LIBRARY_NAME}_CPPFILES ${cls}.cpp)
    endforeach()
    set(${LIBRARY_NAME}_${VISIBILITY}_HEADERS
        ${${LIBRARY_NAME}_${VISIBILITY}_HEADERS}
        PARENT_SCOPE
    )
    set(${LIBRARY_NAME}_CPPFILES ${${LIBRARY_NAME}_CPPFILES} PARENT_SCOPE)
endfunction() # _classes


function(_get_install_dir path out)
    if (PXR_INSTALL_SUBDIR)
        set(${out} ${PXR_INSTALL_SUBDIR}/${path} PARENT_SCOPE)
    else()
        set(${out} ${path} PARENT_SCOPE)
    endif()
endfunction() # get_install_dir

macro(_get_share_install_dir RESULT)
    _get_install_dir(share/usd ${RESULT})
endmacro() # _get_share_install_dir

