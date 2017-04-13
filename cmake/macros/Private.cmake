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

function(_plugInfo_subst libTarget plugInfoPath)
    # PLUG_INFO_LIBRARY_PATH should be set by the caller (see pxr_plugin and
    # pxr_shared_library).
    _get_resources_dir_name(PLUG_INFO_RESOURCE_PATH)
    set(PLUG_INFO_ROOT "..")
    set(PLUG_INFO_PLUGIN_NAME "pxr.${libTarget}")

    configure_file(
        ${plugInfoPath}
        ${CMAKE_CURRENT_BINARY_DIR}/${plugInfoPath}
    )
endfunction() # _plugInfo_subst

# Generate a namespace declaration header, pxr.h at the top level of pxr.
function(_pxrNamespace_subst)
    # Generate the pxr.h file at configuration time
    configure_file(${CMAKE_SOURCE_DIR}/pxr/pxr.h.in
        ${CMAKE_BINARY_DIR}/include/pxr/pxr.h     
    )  

    install(FILES ${CMAKE_BINARY_DIR}/include/pxr/pxr.h
            DESTINATION include/pxr
    )
endfunction()

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
    _get_python_module_name(${LIBRARY_NAME} LIBRARY_INSTALLNAME)

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
    _get_resources_dir(${PLUGINS_PREFIX} ${LIBRARY_NAME} resourcesPath)
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
            DESTINATION ${resourcesPath}/${dirPath}
        )
    endforeach()
endfunction() # _install_resource_files

function(_install_pyside_ui_files)
    set(uiFiles "")
    foreach(uiFile ${ARGN})
        get_filename_component(outFileName ${uiFile} NAME_WE)
        get_filename_component(uiFilePath ${uiFile} ABSOLUTE)
        set(outFilePath "${CMAKE_CURRENT_BINARY_DIR}/${outFileName}.py")
        add_custom_command(
            OUTPUT ${outFilePath}
            COMMAND "${PYSIDEUICBINARY}"
            ARGS -o ${outFilePath} ${uiFilePath}
            MAIN_DEPENDENCY "${uiFilePath}"
            COMMENT "Generating Python for ${uiFilePath} ..."
            VERBATIM
        )
        list(APPEND uiFiles ${outFilePath})
    endforeach()

    add_custom_target(${LIBRARY_NAME}_pysideuifiles ALL
        DEPENDS ${uiFiles}
    )
    set_target_properties(
        ${LIBRARY_NAME}_pysideuifiles
        PROPERTIES
            FOLDER "${PXR_PREFIX}/_pysideuifiles"
    )

    set(libPythonPrefix lib/python)
    _get_python_module_name(${LIBRARY_NAME} LIBRARY_INSTALLNAME)

    install(
        FILES ${uiFiles}
        DESTINATION "${libPythonPrefix}/pxr/${LIBRARY_INSTALLNAME}"
    )
endfunction() # _install_pyside_ui_files

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

function(_get_resources_dir_name output)
    set(${output} 
        resources 
        PARENT_SCOPE)
endfunction() # _get_resources_dir_name

function(_get_plugin_root pluginsPrefix pluginName output)
    set(${output} 
        ${pluginsPrefix}/${pluginName}
        PARENT_SCOPE)
endfunction() # _get_plugin_root

function(_get_resources_dir pluginsPrefix pluginName output)
    _get_resources_dir_name(resourcesDir)
    _get_plugin_root(${pluginsPrefix} ${pluginName} pluginRoot)
    set(${output} 
        ${pluginRoot}/${resourcesDir} 
        PARENT_SCOPE)
endfunction() # _get_resources_dir

function(_get_library_file target output)
    get_target_property(prefix ${target} PREFIX)
    if (NOT prefix AND NOT "" STREQUAL "${prefix}")
        set(prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
    endif()

    get_target_property(suffix ${target} SUFFIX)
    if (NOT suffix AND NOT "" STREQUAL "${suffix}")
        set(suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    set(${output} 
        ${prefix}${target}${suffix}
        PARENT_SCOPE)
endfunction() # _get_library_file

macro(_get_share_install_dir RESULT)
    _get_install_dir(share/usd ${RESULT})
endmacro() # _get_share_install_dir

function(_append_to_rpath orig_rpath new_rpath output)
    # Strip trailing '/' path separators, then append to orig_rpath
    # if new_rpath doesn't already exist.
    string(REGEX REPLACE "/+$" "" new_rpath ${new_rpath})
    string(FIND ${orig_rpath} ${new_rpath} rpath_exists)
    if (rpath_exists EQUAL -1)
        set(${output} "${orig_rpath};${new_rpath}" PARENT_SCOPE)
    endif()
endfunction() # _add_to_rpath

function(_get_directory_property property separator output)
    get_property(value DIRECTORY PROPERTY ${property})
    if(NOT value STREQUAL "value-NOTFOUND")
        # XXX -- Need better list joining.
        if(${output})
            set(${output} "${${output}}${separator}${value}" PARENT_SCOPE)
        else()
            set(${output} "${value}" PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(_get_target_property target property separator output)
    get_property(value TARGET ${target} PROPERTY ${property})
    if(NOT value STREQUAL "value-NOTFOUND")
        # XXX -- Need better list joining.
        if(${output})
            set(${output} "${${output}}${separator}${value}" PARENT_SCOPE)
        else()
            set(${output} "${value}" PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(_get_property target property output)
    set(sep ";")
    if("${property}" STREQUAL "COMPILE_FLAGS")
        set(sep " ")
        set(accum "${accum}${sep}${CMAKE_CXX_FLAGS}")
        if(CMAKE_BUILD_TYPE)
            string(TOUPPER ${CMAKE_BUILD_TYPE} buildType)
            set(accum "${accum}${sep}${CMAKE_CXX_FLAGS_${buildType}}")
        endif()
    endif()
    _get_directory_property(${property} "${sep}" accum)
    _get_target_property(${target} ${property} "${sep}" accum)
    set(${output} "${accum}" PARENT_SCOPE)
endfunction()

function(_pxr_enable_precompiled_header TARGET_NAME)
    # Ignore if disabled.
    if(NOT PXR_ENABLE_PRECOMPILED_HEADERS)
        return()
    endif()

    set(options
    )
    set(oneValueArgs
        SOURCE_NAME
        OUTPUT_NAME_PREFIX
    )
    set(multiValueArgs
        EXCLUDE
    )
    cmake_parse_arguments(pch
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Header to precompile.  SOURCE_NAME falls back to
    # ${PXR_PRECOMPILED_HEADER_NAME}.
    if("${pch_SOURCE_NAME}" STREQUAL "")
        set(pch_SOURCE_NAME "${PXR_PRECOMPILED_HEADER_NAME}")
    endif()
    if("${pch_SOURCE_NAME}" STREQUAL "")
        # Emergency backup name is "pch.h".
        set(pch_SOURCE_NAME "pch.h")
    endif()
    set(source_header_name ${pch_SOURCE_NAME})
    get_filename_component(source_header_name_we ${source_header_name} NAME_WE)

    # Name of file to precompile in the build directory.  The client can
    # specify a prefix for this file, allowing multiple binaries/libraries
    # in a single subdirectory to use unique precompiled headers, meaning
    # each can have different compile options.
    set(output_header_name_we "${pch_OUTPUT_NAME_PREFIX}${source_header_name_we}")
    set(output_header_name ${output_header_name_we}.h)

    # Precompiled header file name.  We choose the name that matches the
    # convention for the compiler.  That isn't necessary since we give
    # this name explicitly wherever it's needed.
    if(MSVC)
        set(precompiled_name ${output_header_name_we}.pch)
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        set(precompiled_name ${output_header_name_we}.h.gch)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(precompiled_name ${output_header_name_we}.h.pch)
    else()
        # Silently ignore unknown compiler.
        return()
    endif()

    # Headers live in subdirectories.
    set(rel_output_header_path "${PXR_PREFIX}/${TARGET_NAME}/${output_header_name}")
    set(abs_output_header_path "${CMAKE_BINARY_DIR}/include/${rel_output_header_path}")
    set(abs_precompiled_path ${CMAKE_BINARY_DIR}/include/${PXR_PREFIX}/${TARGET_NAME}/${precompiled_name})

    # Additional compile flags to use precompiled header.  This will be
    set(compile_flags "")
    if(MSVC)
        # Build with precompiled header (/Yu, /Fp) and automatically
        # include the header (/FI).
        set(compile_flags "/Yu\"${rel_output_header_path}\" /FI\"${rel_output_header_path}\" /Fp\"${abs_precompiled_path}\"")
    else()
        # Automatically include the header (-include) and warn if there's
        # a problem with the precompiled header.
        set(compile_flags "-Winvalid-pch -include \"${rel_output_header_path}\"")
    endif()

    # Use FALSE if we have an external precompiled header we can use.
    if(TRUE)
        if(MSVC)
            # Copy the header to precompile.
            add_custom_command(
                OUTPUT "${abs_output_header_path}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${source_header_name}" "${abs_output_header_path}"
                DEPENDS "${source_header_name}"
                COMMENT "Copying ${source_header_name}"
            )

            # Make an empty trigger file.  We need a source file to do the
            # precompilation.  This file only needs to include the header to
            # precompile but we're implicitly including that header so this
            # file can be empty.
            set(abs_output_source_path ${CMAKE_CURRENT_BINARY_DIR}/${output_header_name_we}.cpp)
            add_custom_command(
                OUTPUT "${abs_output_source_path}"
                COMMAND ${CMAKE_COMMAND} -E touch ${abs_output_source_path}
            )

            # The trigger file gets a special compile flag (/Yc).
            set_source_files_properties(${abs_output_source_path} PROPERTIES
                COMPILE_FLAGS "/Yc\"${rel_output_header_path}\" /FI\"${rel_output_header_path}\" /Fp\"${abs_precompiled_path}\""
                OBJECT_OUTPUTS "${abs_precompiled_path}"
                OBJECT_DEPENDS "${abs_output_header_path}"
            )

            # Add the header file to the target.
            target_sources(${TARGET_NAME} PRIVATE "${abs_output_header_path}")

            # Add the trigger file to the target.
            target_sources(${TARGET_NAME} PRIVATE "${abs_output_source_path}")

            # Exclude the trigger.
            list(APPEND pch_EXCLUDE ${abs_output_source_path})
        else()
            # Copy the header to precompile.
            add_custom_command(
                OUTPUT "${abs_output_header_path}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${source_header_name}" "${abs_output_header_path}"
                DEPENDS "${source_header_name}"
                COMMENT "Copying ${source_header_name}"
            )

            # CMake has no simple way of invoking the compiler with additional
            # arguments so we must make a custom command and pass the compiler
            # arguments we collect here.
            #
            # $<JOIN:...> is available starting with 2.8.12.  In later
            # cmake versions getting the target properties may not
            # report all values (in particular, some include directories
            # may not be reported).
            if(CMAKE_VERSION VERSION_LESS "2.8.12")
                _get_property(${TARGET_NAME} INCLUDE_DIRECTORIES incs)
                _get_property(${TARGET_NAME} COMPILE_DEFINITIONS defs)
                _get_property(${TARGET_NAME} COMPILE_FLAGS flags)
                _get_property(${TARGET_NAME} COMPILE_OPTIONS opts)
                if(NOT "${incs}" STREQUAL "")
                    string(REPLACE ";" ";-I" incs "${incs}")
                    set(incs "-I${incs}")
                endif()
                if(NOT "${defs}" STREQUAL "")
                    string(REPLACE ";" ";-D" defs "${defs}")
                    set(defs "-D${defs}")
                endif()
                separate_arguments(flags UNIX_COMMAND "${flags}")

                # Command to generate the precompiled header.
                add_custom_command(
                    OUTPUT "${abs_precompiled_path}"
                    COMMAND ${CMAKE_CXX_COMPILER} ${flags} ${opts} ${defs} ${incs} -c -x c++-header -o "${abs_precompiled_path}" "${abs_output_header_path}"
                    DEPENDS "${abs_output_header_path}"
                    COMMENT "Precompiling ${source_header_name} in ${TARGET_NAME}"
                )
            else()
                set(incs "$<TARGET_PROPERTY:${TARGET_NAME},INCLUDE_DIRECTORIES>")
                set(defs "$<TARGET_PROPERTY:${TARGET_NAME},COMPILE_DEFINITIONS>")
                set(opts "$<TARGET_PROPERTY:${TARGET_NAME},COMPILE_OPTIONS>")
                set(incs "$<$<BOOL:${incs}>:-I$<JOIN:${incs}, -I>>")
                set(defs "$<$<BOOL:${defs}>:-D$<JOIN:${defs}, -D>>")
                _get_property(${TARGET_NAME} COMPILE_FLAGS flags)

                # Ideally we'd just put have generator expressions in the
                # COMMAND in add_custom_command().  However that will
                # write the result of the JOINs as single strings (escaping
                # spaces) and we want them as individual options.
                #
                # So we use file(GENERATE) which doesn't suffer from that
                # problem and execute the generated cmake script as the
                # COMMAND.
                file(GENERATE
                    OUTPUT "$<TARGET_FILE:${TARGET_NAME}>.pchgen"
                    CONTENT "execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${flags} ${opt} ${defs} ${incs} -c -x c++-header -o \"${abs_precompiled_path}\" \"${abs_output_header_path}\")"
                )

                # Command to generate the precompiled header.
                add_custom_command(
                    OUTPUT "${abs_precompiled_path}"
                    COMMAND ${CMAKE_COMMAND} -P "$<TARGET_FILE:${TARGET_NAME}>.pchgen"
                    DEPENDS "${abs_output_header_path}"
                    COMMENT "Precompiling ${source_header_name} in ${TARGET_NAME}"
                )
            endif()
        endif()
    endif()

    # Update every C++ source in the target to implicitly include and
    # depend on the precompiled header.
    get_property(target_sources TARGET ${TARGET_NAME} PROPERTY SOURCES)
    foreach(source ${target_sources})
        # All target C++ sources not in EXCLUDE list.
        if(source MATCHES \\.cpp$)
            if (NOT ";${pch_EXCLUDE};" MATCHES ";${source};")
                set_source_files_properties(${source} PROPERTIES
                    COMPILE_FLAGS "${compile_flags}"
                    OBJECT_DEPENDS "${abs_precompiled_path}")
            endif()
        endif()
    endforeach()
endfunction()
