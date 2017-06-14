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
            if(PXR_SYMLINK_HEADER_FILES AND NOT WIN32)
                # cmake -E create_symlink doesn't create parent directories, while
                # copy does... so need an extra command to make dir

                # Also, if ${f} has a directory, header_parent_dir will be
                # different than header_dest_dir                
                get_filename_component(header_parent_dir ${outfile} DIRECTORY)
                add_custom_command(
                        OUTPUT ${outfile}
                        COMMAND "${CMAKE_COMMAND}"
                        ARGS -E make_directory "${header_parent_dir}"
                        COMMAND "${CMAKE_COMMAND}"
                        ARGS -E create_symlink "${infile}" "${outfile}"
                        MAIN_DEPENDENCY "${infile}"
                        COMMENT "Symlinking ${f} ..."
                        VERBATIM
                )
            else()
                add_custom_command(
                OUTPUT ${outfile}
                        COMMAND
                            "${PYTHON_EXECUTABLE}"
                            "${PROJECT_SOURCE_DIR}/cmake/macros/copyHeaderForBuild.py"
                            "${infile}"
                            "${outfile}"
                        MAIN_DEPENDENCY "${infile}"
                        COMMENT "Copying ${f} ..."
                        VERBATIM
                )
            endif()
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

function(_plugInfo_subst libTarget pluginToLibraryPath plugInfoPath)
    _get_resources_dir_name(PLUG_INFO_RESOURCE_PATH)
    set(PLUG_INFO_ROOT "..")
    set(PLUG_INFO_PLUGIN_NAME "pxr.${libTarget}")
    set(PLUG_INFO_LIBRARY_PATH "${pluginToLibraryPath}")

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

# Generate a doxygen config file
function(_pxrDoxyConfig_subst)
    configure_file(${CMAKE_SOURCE_DIR}/pxr/usd/lib/usd/Doxyfile.in
                   ${CMAKE_BINARY_DIR}/Doxyfile
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
        elseif (${file} MATCHES ".qss$")
            # XXX -- Allow anything or allow nothing?
            list(APPEND filesToInstall ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        else()
            message(FATAL_ERROR "Cannot have non-Python file ${file} in PYTHON_FILES.")
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
        DEPENDS ${files_copied}
    )

    _get_folder("_python" folder)
    set_target_properties(${LIBRARY_NAME}_pythonfiles
        PROPERTIES
            FOLDER "${folder}"
    )
endfunction() #_install_python

function(_install_resource_files NAME pluginInstallPrefix pluginToLibraryPath)
    set(resourceFiles "")
    foreach(resourceFile ${ARGN})
        # plugInfo.json go through an initial template substitution step files
        # install it from the binary (gen) directory specified by the full
        # path. Otherwise, use the original relative path which is relative to
        # the source directory.
        if (${resourceFile} STREQUAL "plugInfo.json")
            _plugInfo_subst(${NAME} "${pluginToLibraryPath}" ${resourceFile})
            list(APPEND resourceFiles "${CMAKE_CURRENT_BINARY_DIR}/${resourceFile}")
        else()
            list(APPEND resourceFiles ${resourceFile})
        endif()
    endforeach()

    # Resource files install into a structure that looks like:
    # share/
    #     usd/
    #         ${NAME}/
    #             resources/
    #                 resourceFileA
    #                 subdir/
    #                     resourceFileB
    #                     resourceFileC
    #                 ...
    #
    _get_resources_dir(${pluginInstallPrefix} ${NAME} resourcesPath)
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

function(_install_pyside_ui_files LIBRARY_NAME)
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

    _get_folder("_pysideuifiles" folder)
    set_target_properties(
        ${LIBRARY_NAME}_pysideuifiles
        PROPERTIES
            FOLDER "${folder}"
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

function(_get_folder suffix result)
    # XXX -- Shouldn't we set PXR_PREFIX everywhere?
    if(PXR_PREFIX)
        set(folder "${PXR_PREFIX}")
    elseif(PXR_INSTALL_SUBDIR)
        set(folder "${PXR_INSTALL_SUBDIR}")
    else()
        set(folder "misc")
    endif()
    if(suffix)
        set(folder "${folder}/${suffix}")
    endif()
    set(${result} ${folder} PARENT_SCOPE)
endfunction()

function(_pch_get_directory_property property separator output)
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

function(_pch_get_target_property target property separator output)
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

function(_pch_get_property target property output)
    set(sep ";")
    if("${property}" STREQUAL "COMPILE_FLAGS")
        set(sep " ")
        set(accum "${accum}${sep}${CMAKE_CXX_FLAGS}")
        if(CMAKE_BUILD_TYPE)
            string(TOUPPER ${CMAKE_BUILD_TYPE} buildType)
            set(accum "${accum}${sep}${CMAKE_CXX_FLAGS_${buildType}}")
        endif()
    endif()
    _pch_get_directory_property(${property} "${sep}" accum)
    _pch_get_target_property(${target} ${property} "${sep}" accum)
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
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
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
                _pch_get_property(${TARGET_NAME} INCLUDE_DIRECTORIES incs)
                _pch_get_property(${TARGET_NAME} COMPILE_DEFINITIONS defs)
                _pch_get_property(${TARGET_NAME} COMPILE_FLAGS flags)
                _pch_get_property(${TARGET_NAME} COMPILE_OPTIONS opts)
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
                _pch_get_property(${TARGET_NAME} COMPILE_FLAGS flags)

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

# Initialize a variable to accumulate an rpath.  The origin is the
# RUNTIME DESTINATION of the target.  If not absolute it's appended
# to CMAKE_INSTALL_PREFIX.
function(_pxr_init_rpath rpathRef origin)
    if(NOT IS_ABSOLUTE ${origin})
        set(origin "${CMAKE_INSTALL_PREFIX}/${origin}")
        get_filename_component(origin "${origin}" REALPATH)
    endif()
    set(${rpathRef} "${origin}" PARENT_SCOPE)
endfunction()

# Add a relative target path to the rpath.  If target is absolute compute
# and add a relative path from the origin to the target.
function(_pxr_add_rpath rpathRef target)
    if(IS_ABSOLUTE "${target}")
        # Make target relative to $ORIGIN (which is the first element in
        # rpath when initialized with _pxr_init_rpath()).
        list(GET ${rpathRef} 0 origin)
        file(RELATIVE_PATH
            target
            "${origin}"
            "${target}"
        )
        if("x${target}" STREQUAL "x")
            set(target ".")
        endif()
    endif()
    file(TO_CMAKE_PATH "${target}" target)
    set(new_rpath "${${rpathRef}}")
    list(APPEND new_rpath "$ORIGIN/${target}")
    set(${rpathRef} "${new_rpath}" PARENT_SCOPE)
endfunction()

function(_pxr_install_rpath rpathRef NAME)
    # Get and remove the origin.
    list(GET ${rpathRef} 0 origin)
    set(rpath ${${rpathRef}})
    list(REMOVE_AT rpath 0)

    # Canonicalize and uniquify paths.
    set(final "")
    foreach(path ${rpath})
        # Absolutize on Mac.  SIP disallows relative rpaths.
        if(APPLE)
            if("${path}/" MATCHES "^[$]ORIGIN/")
                # Replace with origin path.
                string(REPLACE "$ORIGIN/" "${origin}/" path "${path}/")

                # Simplify.
                get_filename_component(path "${path}" REALPATH)
            endif()
        endif()

        # Strip trailing slashes.
        string(REGEX REPLACE "/+$" "" path "${path}")

        # Ignore paths we already have.
        if (NOT ";${final};" MATCHES ";${path};")
            list(APPEND final "${path}")
        endif()
    endforeach()

    set_target_properties(${NAME}
        PROPERTIES 
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${final}"
    )
endfunction()

# Split the library (target) names in libs into internal (in-build) and
# external library lists.
function(_pxr_split_libraries libs internal_result external_result)
    set(internal "")
    set(external "")
    foreach(lib ${libs})
        if(";${PXR_ALL_LIBS};" MATCHES ";${lib};")
            list(APPEND internal "${lib}")
        else()
            list(APPEND external "${lib}")
        endif()
    endforeach()
    set(${internal_result} ${internal} PARENT_SCOPE)
    set(${external_result} ${external} PARENT_SCOPE)
endfunction()

# Helper functions for _pxr_transitive_internal_libraries.
function(_pxr_transitive_internal_libraries_r libs transitive_libs)
    set(result "${${transitive_libs}}")
    foreach(lib ${libs})
        # Handle library ${lib} only if it hasn't been seen yet.
        if(NOT ";${result};" MATCHES ";${lib};")
            # Add to result.
            list(APPEND result ${lib})

            # Get the implicit link libraries.
            get_property(implicit TARGET ${lib} PROPERTY INTERFACE_LINK_LIBRARIES)

            # Discard the external libraries.
            _pxr_split_libraries("${implicit}" internal external)

            # Recurse on the internal libraries.
            _pxr_transitive_internal_libraries_r("${internal}" result)
        endif()
    endforeach()
    set(${transitive_libs} "${result}" PARENT_SCOPE)
endfunction()

function(_pxr_transitive_internal_libraries libs transitive_libs)
    # Get the transitive libs in some order.
    set(transitive "")
    _pxr_transitive_internal_libraries_r("${libs}" transitive)

    # Get the transitive libs in build order.
    set(result "")
    foreach(lib ${PXR_ALL_LIBS})
        if(";${transitive};" MATCHES ";${lib};")
            list(APPEND result "${lib}")
        endif()
    endforeach()

    # Reverse the order to get the link order.
    list(REVERSE result)
    set(${transitive_libs} "${result}" PARENT_SCOPE)
endfunction()

function(_pxr_target_link_libraries NAME)
    # Split internal (in-build) libraries from external libraries.
    _pxr_split_libraries("${ARGN}" internal external)

    # Adjust link line.
    if(PXR_BUILD_MONOLITHIC)
        # If we use any internal libraries then just link against the
        # monolithic library.
        if(internal)
            set(internal shared_monolithic)
        endif()
    elseif(NOT BUILD_SHARED_LIBS)
        # Indicate that all symbols should be pulled in from internal
        # static libraries.  This ensures we don't drop unused symbols
        # with dynamic initialization side effects.  The exceptions are
        # any libraries explicitly static;  not only does that explicitly
        # say we don't have to worry about the dynamic initialization, but
        # also would maybe cause multiple symbol definitions if we tried
        # to get all symbols.
        #
        # On gcc use: --whole_archive LIB --no-whole-archive.
        # On clang use: -force_load LIB
        # On Windows use: /WHOLEARCHIVE:LIB
        #
        # A final complication is that we must also process transitive
        # link libraries since any transitively linked internal libraries
        # need the same treatment.
        _pxr_transitive_internal_libraries("${internal}" internal)
        set(final "")
        foreach(lib ${internal})
            if(";${PXR_STATIC_LIBS};" MATCHES ";${lib};")
                # The library is explicitly static.
                list(APPEND final ${lib})
            elseif(MSVC)
                # The syntax here is -WHOLEARCHIVE[:lib] but CMake will
                # treat that as a link flag and not "see" the library.
                # As a result it won't replace a target with the path
                # to the built target and it won't add a dependency.
                #
                # We can't simply link against the full path to the
                # library because we CMake will not add a dependency
                # and won't use interface link libraries and flags
                # from the targets.  Rather than trying to add those
                # things manually we instead link against the target
                # and link against the full path to the built target
                # with WHOLEARCHIVE.
                #
                # This ends up with the library on the link line twice.
                # That's okay, though, because the linker will read
                # the WHOLEARCHIVE one first and will use none of the
                # (duplicate) symbols from the second since they're
                # all provided by the first.  The order doesn't really
                # matter; we pull in the whole archive first.
                #
                list(APPEND final -WHOLEARCHIVE:$<TARGET_FILE:${lib}>)
                list(APPEND final ${lib})
            elseif(CMAKE_COMPILER_IS_GNUCXX)
                list(APPEND final -Wl,--whole-archive ${lib} -Wl,--no-whole-archive)
            elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
                list(APPEND final -Wl,-force_load ${lib})
            else()
                # Unknown platform.
                list(APPEND final ${lib})
            endif()
        endforeach()
        set(internal ${final})
    endif()
    target_link_libraries(${NAME}
        ${internal}
        ${external}
        ${PXR_THREAD_LIBS}
    )
endfunction()

# Add a python module for the target named NAME.  It implicitly links
# against the library named NAME (or the monolithic library if
# PXR_BUILD_MONOLITHIC is enabled).
function(_pxr_python_module NAME)
    set(oneValueArgs
        PRECOMPILED_HEADERS
        PRECOMPILED_HEADER_NAME
        WRAPPED_LIB_INSTALL_PREFIX
    )
    set(multiValueArgs
        CPPFILES
        PYTHON_FILES
        PYSIDE_UI_FILES
        INCLUDE_DIRS
    )
    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # If we can't build Python modules then do nothing.
    if(NOT BUILD_SHARED_LIBS AND NOT TARGET shared_monolithic)
        message(STATUS "Skipping Python module ${NAME}, shared libraries required")
        return()
    endif()

    set(LIBRARY_NAME "_${NAME}")

    # Install .py files.
    if(args_PYTHON_FILES)
        _install_python(${LIBRARY_NAME}
            FILES ${args_PYTHON_FILES}
        )
    endif()

    # Install .ui files.
    if (args_PYSIDE_UI_FILES)
        _install_pyside_ui_files(${LIBRARY_NAME} ${args_PYSIDE_UI_FILES})
    endif()        

    # If no C++ files then we're done.
    if (NOT args_CPPFILES)
        return()
    endif()

    add_library(${LIBRARY_NAME}
        SHARED
        ${args_CPPFILES}
    )

    # Convert the name of the library into the python module name
    # , e.g. _tf.so -> Tf. This is later used to determine the eventual
    # install location as well as for inclusion into the __init__.py's 
    # __all__ list.
    _get_python_module_name(${LIBRARY_NAME} pyModuleName)

    # Accumulate Python module names.
    set_property(GLOBAL
        APPEND PROPERTY PXR_PYTHON_MODULES ${pyModuleName}
    )

    # Always install under the 'pxr' module, rather than base on the
    # project name. This makes importing consistent, e.g. 
    # 'from pxr import X'. Additionally, python libraries always install
    # into the default lib install, not into the third_party subdirectory
    # or similar.
    set(LIB_INSTALL_PREFIX "lib/python/pxr/${pyModuleName}")

    # Python modules need to be able to access their corresponding
    # wrapped library and the install lib directory.
    _pxr_init_rpath(rpath "${LIB_INSTALL_PREFIX}")
    _pxr_add_rpath(rpath
        "${CMAKE_INSTALL_PREFIX}/${args_WRAPPED_LIB_INSTALL_PREFIX}")
    _pxr_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    _pxr_install_rpath(rpath ${LIBRARY_NAME})

    _get_folder("_python" folder)
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES
            PREFIX ""
            FOLDER "${folder}"
    )
    if(WIN32)
        # Python modules must be suffixed with .pyd on Windows.
        set_target_properties(${LIBRARY_NAME}
            PROPERTIES
                SUFFIX ".pyd"
        )
    elseif(APPLE)
        # Python modules must be suffixed with .so on Mac.
        set_target_properties(${LIBRARY_NAME}
            PROPERTIES
                SUFFIX ".so"
        )
    endif()

    target_compile_definitions(${LIBRARY_NAME}
        PRIVATE
            MFB_PACKAGE_NAME=${PXR_PACKAGE}
            MFB_ALT_PACKAGE_NAME=${PXR_PACKAGE}
            MFB_PACKAGE_MODULE=${pyModuleName}
    )

    _pxr_target_link_libraries(${LIBRARY_NAME}
        ${NAME}
        ${PXR_MALLOC_LIBRARY}
    )

    # Include headers from the build directory.
    get_filename_component(
        PRIVATE_INC_DIR
        "${CMAKE_BINARY_DIR}/include"
        ABSOLUTE
    )
    if (PXR_INSTALL_SUBDIR)
        get_filename_component(
            SUBDIR_INC_DIR
            "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}/include"
            ABSOLUTE
        )
    endif()
    target_include_directories(${LIBRARY_NAME}
        PRIVATE
            ${PRIVATE_INC_DIR}
            ${SUBDIR_INC_DIR}
    )

    # Include system headers before our own.  We define several headers
    # that conflict; for example, half.h in EXR versus gf
    if (args_INCLUDE_DIRS)
        target_include_directories(${LIBRARY_NAME}
            BEFORE
            PUBLIC
                ${args_INCLUDE_DIRS}
        )
    endif()

    install(TARGETS ${LIBRARY_NAME}
        EXPORT pxrTargets
        LIBRARY DESTINATION ${LIB_INSTALL_PREFIX}
        ARCHIVE DESTINATION ${LIB_INSTALL_PREFIX}
        RUNTIME DESTINATION ${LIB_INSTALL_PREFIX}
    )

    export(TARGETS ${LIBRARY_NAME}
        APPEND
        FILE "${PROJECT_BINARY_DIR}/pxrTargets.cmake"
    )
    
    if(NOT "${PXR_PREFIX}" STREQUAL "")
        if(args_PRECOMPILED_HEADERS)
            _pxr_enable_precompiled_header(${LIBRARY_NAME}
                OUTPUT_NAME_PREFIX "py"
                SOURCE_NAME "${args_PRECOMPILED_HEADER_NAME}"
            )
        endif()
    endif()
endfunction() # pxr_python_module

# Add a library target named NAME.
function(_pxr_library NAME)
    # Argument parsing.
    set(options
    )
    set(oneValueArgs
        PREFIX
        SUBDIR
        SUFFIX
        TYPE
        PRECOMPILED_HEADERS
        PRECOMPILED_HEADER_NAME
    )
    set(multiValueArgs
        PUBLIC_HEADERS
        PRIVATE_HEADERS
        CPPFILES
        LIBRARIES
        INCLUDE_DIRS
        RESOURCE_FILES
        LIB_INSTALL_PREFIX_RESULT
    )
    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Generate namespace header for pxr.
    _pxrNamespace_subst()

    #
    # Set up the target.
    #

    # Add the target.  We also add the headers because that's the easiest
    # way to get them to appear in IDE projects.
    if(args_TYPE STREQUAL "STATIC")
        add_library(${NAME}
            STATIC
            ${args_CPPFILES}
            ${args_PUBLIC_HEADERS}
            ${args_PRIVATE_HEADERS}
        )
    else()
        add_library(${NAME}
            SHARED
            ${args_CPPFILES}
            ${args_PUBLIC_HEADERS}
            ${args_PRIVATE_HEADERS}
        )
    endif()

    #
    # Compute names and paths.
    #

    # Where do we install to?
    _get_install_dir("include/${PXR_PREFIX}/${NAME}" headerInstallPrefix)
    if(args_TYPE STREQUAL "PLUGIN")
        _get_install_dir("plugin" libInstallPrefix)
        if(NOT PXR_INSTALL_SUBDIR)
            # XXX -- Why this difference?
            _get_install_dir("plugin/usd" libInstallPrefix)
        endif()
        set(pluginInstallPrefix "${libInstallPrefix}")
    else()
        _get_install_dir("lib" libInstallPrefix)
        _get_install_dir("share/usd/plugins" pluginInstallPrefix)
    endif()
    if(args_SUBDIR)
        set(libInstallPrefix "${libInstallPrefix}/${args_SUBDIR}")
        set(pluginInstallPrefix "${pluginInstallPrefix}/${args_SUBDIR}")
    endif()
    # Return libInstallPrefix to caller.
    if(args_LIB_INSTALL_PREFIX_RESULT)
        set(${args_LIB_INSTALL_PREFIX_RESULT} "${libInstallPrefix}" PARENT_SCOPE)
    endif()

    # Names and paths passed to the compile via macros.  Paths should be
    # relative to facilitate relocating the build.  installLocation is
    # absolute but the client can override with any path, including a
    # relative one.
    _get_python_module_name(${NAME} pythonModuleName)
    string(TOUPPER ${NAME} uppercaseName)
    set(installLocation "${CMAKE_INSTALL_PREFIX}/share/usd/plugins")
    if(PXR_INSTALL_LOCATION)
        file(TO_CMAKE_PATH "${PXR_INSTALL_LOCATION}" installLocation)
    endif()

    # API macros.
    set(apiPublic "")
    set(apiPrivate ${uppercaseName}_EXPORTS=1)
    if(args_TYPE STREQUAL "STATIC")
        set(apiPublic PXR_STATIC=1)
    endif()

    # Final name.
    set(libraryFilename "${args_PREFIX}${NAME}${args_SUFFIX}")

    # Figure out the relative path from this library's plugin location
    # (in the libplug sense, which applies even to non-plugins, and is
    # where we can find external resources for the library) to the
    # library's location.  This can be embedded into resource files.
    file(RELATIVE_PATH 
        pluginToLibraryPath 
        ${CMAKE_INSTALL_PREFIX}/${pluginInstallPrefix}/${NAME}
        ${CMAKE_INSTALL_PREFIX}/${libInstallPrefix}/${libraryFilename})

    #
    # Set up the compile/link.
    #

    # PIC is required by shared libraries. It's on for static libraries
    # because we'll likely link them into a shared library.
    _get_folder("" folder)
    set_target_properties(${NAME}
        PROPERTIES
            FOLDER "${folder}"
            POSITION_INDEPENDENT_CODE ON
            PREFIX "${args_PREFIX}"
            SUFFIX "${args_SUFFIX}"
            PUBLIC_HEADER "${args_PUBLIC_HEADERS}"
    )

    set(pythonEnabled "PXR_PYTHON_ENABLED=1")
    if(BUILD_SHARED_LIBS OR TARGET shared_monolithic)
        set(pythonModulesEnabled "PXR_PYTHON_MODULES_ENABLED=1")
    endif()
    target_compile_definitions(${NAME}
        PUBLIC
            ${pythonEnabled}
            ${apiPublic}
        PRIVATE
            MFB_PACKAGE_NAME=${PXR_PACKAGE}
            MFB_ALT_PACKAGE_NAME=${PXR_PACKAGE}
            MFB_PACKAGE_MODULE=${pythonModuleName}
            "PXR_BUILD_LOCATION=../share/usd/plugins"
            "PXR_PLUGIN_BUILD_LOCATION=../plugin/usd"
            "PXR_INSTALL_LOCATION=${installLocation}"
            ${pythonModulesEnabled}
            ${apiPrivate}
    )

    # Include headers from the build directory, the subdir (3rd party)
    # build directory, then external packages.
    target_include_directories(${NAME}
        PRIVATE
            "${CMAKE_BINARY_DIR}/include"
            "${CMAKE_BINARY_DIR}/${PXR_INSTALL_SUBDIR}/include"
        PUBLIC
            ${args_INCLUDE_DIRS}
    )

    # XXX -- May want some plugins to be baked into monolithic.
    if(args_TYPE STREQUAL "PLUGIN")
        _pxr_target_link_libraries(${NAME}
            ${args_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
        )
    else()
        # Just the standard target_link_libraries() because either we're
        # building a shared library and it doesn't matter or we're building
        # a static library and that doesn't actually link anything (it just
        # builds an archive).
        target_link_libraries(${NAME}
            ${args_LIBRARIES}
            ${PXR_MALLOC_LIBRARY}
            ${PXR_THREAD_LIBS}
        )
    endif()

    # Rpath has libraries under the third party prefix and the install prefix.
    # The former is for helper libraries for a third party application and
    # the latter for core USD libraries.
    _pxr_init_rpath(rpath "${libInstallPrefix}")
    _pxr_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/${PXR_INSTALL_SUBDIR}/lib")
    _pxr_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    _pxr_install_rpath(rpath ${NAME})

    #
    # Set up the install.
    #

    install(TARGETS ${NAME}
        EXPORT pxrTargets
        LIBRARY DESTINATION ${libInstallPrefix}
        ARCHIVE DESTINATION ${libInstallPrefix}
        RUNTIME DESTINATION ${libInstallPrefix}
        PUBLIC_HEADER DESTINATION ${headerInstallPrefix}
    )
    _install_headers(${NAME}
        FILES
            ${args_PUBLIC_HEADERS}
            ${args_PRIVATE_HEADERS}
        PREFIX
            ${PXR_PREFIX}
    )
    if (args_RESOURCE_FILES)
        _install_resource_files(
            ${NAME}
            "${pluginInstallPrefix}"
            "${pluginToLibraryPath}"
            ${args_RESOURCE_FILES})
    endif()
    export(TARGETS ${NAME}
        APPEND
        FILE "${PROJECT_BINARY_DIR}/pxrTargets.cmake"
    )

    # If building monolithic and that target doesn't have the public
    # header directory then add it.
    if(PXR_BUILD_MONOLITHIC)
        get_property(incs TARGET monolithic PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES
        )
        if(NOT ";${incs};" MATCHES ";${headerInstallPrefix};")
            set_property(TARGET monolithic APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES ${headerInstallPrefix}
            )
        endif()
    endif()

    #
    # Set up precompiled headers.
    #

    if(NOT "${PXR_PREFIX}" STREQUAL "")
        if(args_PRECOMPILED_HEADERS)
            _pxr_enable_precompiled_header(${NAME}
                SOURCE_NAME "${args_PRECOMPILED_HEADER_NAME}"
            )
        endif()
    endif()
endfunction() # _pxr_library
