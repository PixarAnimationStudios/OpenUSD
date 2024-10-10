set(PXR_PREFIX pxr/external/boost)
set(PXR_PACKAGE python)

set(libs "")
set(include_dirs "")
set(cppfiles "")

# If we're building our own pxr_boost::python library, then we need
# to specify the source files as well as our dependency on Python.
# Otherwise, this library is just a pass-through for boost::python.
if (PXR_USE_INTERNAL_BOOST_PYTHON)
    # Use PYTHON_LIBRARIES instead of the Python3::Python target
    # directly when specifying library dependencies to ensure
    # PXR_PY_UNDEFINED_DYNAMIC_LOOKUP is respected.
    set(libs
        ${PYTHON_LIBRARIES}
    )

    # Specify PYTHON_INCLUDE_DIRS explicitly to ensure this library
    # picks up Python headers when PXR_PY_UNDEFINED_DYNAMIC_LOOKUP
    # is set.
    set(include_dirs
        ${PYTHON_INCLUDE_DIRS}
    )

    set(cppfiles
        src/converter/arg_to_python_base.cpp
        src/converter/builtin_converters.cpp
        src/converter/from_python.cpp
        src/converter/registry.cpp
        src/converter/type_id.cpp
        src/dict.cpp
        src/errors.cpp
        src/exec.cpp
        src/import.cpp
        src/list.cpp
        src/long.cpp
        src/module.cpp
        src/object/class.cpp
        src/object/enum.cpp
        src/object/function.cpp
        src/object/function_doc_signature.cpp
        src/object/inheritance.cpp
        src/object/iterator.cpp
        src/object/life_support.cpp
        src/object/pickle_support.cpp
        src/object/stl_iterator.cpp
        src/object_operators.cpp
        src/object_protocol.cpp
        src/slice.cpp
        src/str.cpp
        src/tuple.cpp
        src/wrapper.cpp

        # These source files are for boost::python's numpy extension.
        # We do not build these because we don't use this extension
        # and don't want to add a numpy dependency to the build.
        #
        # src/numpy/dtype.cpp
        # src/numpy/matrix.cpp
        # src/numpy/ndarray.cpp
        # src/numpy/numpy.cpp
        # src/numpy/scalars.cpp
        # src/numpy/ufunc.cpp
    )

    # By default CMake defines NDEBUG in CMAKE_CXX_FLAGS_<config> for
    # non-debug build configurations, which applies to all targets
    # created in this directory. This disables assert calls in unit
    # tests, rendering them useless in those configurations.
    #
    # To avoid this, we remove NDEBUG from CMAKE_CXX_FLAGS_<config>
    # and reapply it only to the main library itself below.
    if (MSVC)
        set(ndebug_flag "/DNDEBUG")
    else()
        set(ndebug_flag "-DNDEBUG")
    endif()

    foreach(config RELEASE RELWITHDEBINFO MINSIZEREL)
        string(REPLACE
            ${ndebug_flag} ""
            CMAKE_CXX_FLAGS_${config} ${CMAKE_CXX_FLAGS_${config}}
        )
    endforeach()

else()
    set(libs
        ${Boost_PYTHON_LIBRARY}
    )

    set(include_dirs
        ${Boost_INCLUDE_DIRS}
    )

    # Workaround lack of build support for header-only libs.
    set(cppfiles 
        build-workaround.cpp
    )
endif()

pxr_library(python
    LIBRARIES
        boost
        ${libs}

    INCLUDE_DIRS
        ${include_dirs}

    PUBLIC_HEADERS
        arg_from_python.hpp
        args.hpp
        args_fwd.hpp
        back_reference.hpp
        base_type_traits.hpp
        bases.hpp
        borrowed.hpp
        call.hpp
        call_method.hpp
        cast.hpp
        class.hpp
        class_fwd.hpp
        common.hpp
        converter/arg_from_python.hpp
        converter/arg_to_python.hpp
        converter/arg_to_python_base.hpp
        converter/as_to_python_function.hpp
        converter/builtin_converters.hpp
        converter/constructor_function.hpp
        converter/context_result_converter.hpp
        converter/convertible_function.hpp
        converter/from_python.hpp
        converter/implicit.hpp
        converter/obj_mgr_arg_from_python.hpp
        converter/object_manager.hpp
        converter/pointer_type_id.hpp
        converter/pyobject_traits.hpp
        converter/pyobject_type.hpp
        converter/pytype_function.hpp
        converter/pytype_object_mgr_traits.hpp
        converter/registered.hpp
        converter/registered_pointee.hpp
        converter/registrations.hpp
        converter/registry.hpp
        converter/return_from_python.hpp
        converter/rvalue_from_python_data.hpp
        converter/shared_ptr_deleter.hpp
        converter/shared_ptr_from_python.hpp
        converter/shared_ptr_to_python.hpp
        converter/to_python_function_type.hpp
        copy_const_reference.hpp
        copy_non_const_reference.hpp
        data_members.hpp
        def.hpp
        def_visitor.hpp
        default_call_policies.hpp
        detail/aix_init_module.hpp
        detail/api_placeholder.hpp
        detail/borrowed_ptr.hpp
        detail/caller.hpp
        detail/config.hpp
        detail/construct.hpp
        detail/convertible.hpp
        detail/copy_ctor_mutates_rhs.hpp
        detail/cv_category.hpp
        detail/dealloc.hpp
        detail/decorated_type_id.hpp
        detail/decref_guard.hpp
        detail/def_helper.hpp
        detail/def_helper_fwd.hpp
        detail/defaults_def.hpp
        detail/defaults_gen.hpp
        detail/dependent.hpp
        detail/destroy.hpp
        detail/enable_if.hpp
        detail/exception_handler.hpp
        detail/force_instantiate.hpp
        detail/get_pointer.hpp
        detail/if_else.hpp
        detail/indirect_traits.hpp
        detail/integer_cast.hpp
        detail/invoke.hpp
        detail/is_auto_ptr.hpp
        detail/is_shared_ptr.hpp
        detail/is_wrapper.hpp
        detail/is_xxx.hpp
        detail/make_keyword_range_fn.hpp
        detail/map_entry.hpp
        detail/mpl_lambda.hpp
        detail/mpl2/and.hpp
        detail/mpl2/at.hpp
        detail/mpl2/bool.hpp
        detail/mpl2/eval_if.hpp
        detail/mpl2/front.hpp
        detail/mpl2/identity.hpp
        detail/mpl2/if.hpp
        detail/mpl2/int.hpp
        detail/mpl2/not.hpp
        detail/mpl2/pop_front.hpp
        detail/mpl2/push_front.hpp
        detail/mpl2/or.hpp
        detail/mpl2/size.hpp
        detail/msvc_typeinfo.hpp
        detail/none.hpp
        detail/not_specified.hpp
        detail/nullary_function_adaptor.hpp
        detail/operator_id.hpp
        detail/overloads_fwd.hpp
        detail/pointee.hpp
        detail/prefix.hpp
        detail/preprocessor.hpp
        detail/python_type.hpp
        detail/raw_pyobject.hpp
        detail/referent_storage.hpp
        detail/result.hpp
        detail/scope.hpp
        detail/sfinae.hpp
        detail/signature.hpp
        detail/string_literal.hpp
        detail/target.hpp
        detail/translate_exception.hpp
        detail/type_list.hpp
        detail/type_list_impl.hpp
        detail/type_traits.hpp
        detail/unwind_type.hpp
        detail/unwrap_type_id.hpp
        detail/unwrap_wrapper.hpp
        detail/value_arg.hpp
        detail/value_is_shared_ptr.hpp
        detail/value_is_xxx.hpp
        detail/void_ptr.hpp
        detail/void_return.hpp
        detail/wrap_python.hpp
        detail/wrapper_base.hpp
        dict.hpp
        docstring_options.hpp
        enum.hpp
        errors.hpp
        exception_translator.hpp
        exec.hpp
        extract.hpp
        handle.hpp
        handle_fwd.hpp
        has_back_reference.hpp
        implicit.hpp
        import.hpp
        init.hpp
        instance_holder.hpp
        iterator.hpp
        list.hpp
        long.hpp
        lvalue_from_pytype.hpp
        make_constructor.hpp
        make_function.hpp
        manage_new_object.hpp
        module.hpp
        module_init.hpp
        noncopyable.hpp
        object/add_to_namespace.hpp
        object/class.hpp
        object/class_detail.hpp
        object/class_metadata.hpp
        object/class_wrapper.hpp
        object/enum_base.hpp
        object/find_instance.hpp
        object/forward.hpp
        object/function.hpp
        object/function_doc_signature.hpp
        object/function_handle.hpp
        object/function_object.hpp
        object/inheritance.hpp
        object/inheritance_query.hpp
        object/instance.hpp
        object/iterator.hpp
        object/iterator_core.hpp
        object/life_support.hpp
        object/make_holder.hpp
        object/make_instance.hpp
        object/make_ptr_instance.hpp
        object/pickle_support.hpp
        object/pointer_holder.hpp
        object/py_function.hpp
        object/stl_iterator_core.hpp
        object/value_holder.hpp
        object/value_holder_fwd.hpp
        object.hpp
        object_attributes.hpp
        object_core.hpp
        object_fwd.hpp
        object_items.hpp
        object_operators.hpp
        object_protocol.hpp
        object_protocol_core.hpp
        object_slices.hpp
        opaque_pointer_converter.hpp
        operators.hpp
        other.hpp
        overloads.hpp
        override.hpp
        pointee.hpp
        proxy.hpp
        ptr.hpp
        pure_virtual.hpp
        raw_function.hpp
        ref.hpp
        refcount.hpp
        reference_existing_object.hpp
        register_ptr_to_python.hpp
        return_arg.hpp
        return_by_value.hpp
        return_internal_reference.hpp
        return_opaque_pointer.hpp
        return_value_policy.hpp
        scope.hpp
        self.hpp
        signature.hpp
        slice.hpp
        slice_nil.hpp
        ssize_t.hpp
        stl_iterator.hpp
        str.hpp
        suite/indexing/container_utils.hpp
        suite/indexing/detail/indexing_suite_detail.hpp
        suite/indexing/indexing_suite.hpp
        suite/indexing/map_indexing_suite.hpp
        suite/indexing/vector_indexing_suite.hpp
        tag.hpp
        to_python_converter.hpp
        to_python_indirect.hpp
        to_python_value.hpp
        tuple.hpp
        type.hpp
        type_id.hpp
        type_list.hpp
        with_custodian_and_ward.hpp
        wrapper.hpp

        # These source files are for boost::python's numpy extension.
        # We do not build these because we don't use this extension
        # and don't want to add a numpy dependency to the build.
        #
        # numpy/config.hpp
        # numpy/dtype.hpp
        # numpy/internal.hpp
        # numpy/invoke_matching.hpp
        # numpy/matrix.hpp
        # numpy/ndarray.hpp
        # numpy/numpy_object_mgr_traits.hpp
        # numpy/scalars.hpp
        # numpy/ufunc.hpp
        # numpy.hpp

    CPPFILES
        ${cppfiles}

    DISABLE_PRECOMPILED_HEADERS
)

# Disable insertion of C++ signatures in docstrings. We generate this
# information separately via our Python doc build process.
target_compile_definitions(python
    PUBLIC PXR_BOOST_PYTHON_NO_PY_SIGNATURES
)

if (PXR_USE_INTERNAL_BOOST_PYTHON)
    # Add PXR_BOOST_PYTHON_SOURCE when building this library to ensure symbols
    # decorated with PXR_BOOST_PYTHON_DECL are exported.
    target_compile_definitions(python
        PRIVATE PXR_BOOST_PYTHON_SOURCE
    )

    # See comment about NDEBUG above.
    target_compile_definitions(python
        PRIVATE $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )
else()
    # Ensure symbol in build-workaround.cpp is exported on Windows,
    # otherwise the build will error out at link time because no
    # .lib file will be generated. Using this setting is OK because
    # we only expect there to be one exportable symbol in this library.
    set_target_properties(python
        PROPERTIES 
            WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    )

    # Skip building tests, since in this case this library doesn't
    # have any functionality on its own -- it's just a pass-through
    # for boost::python.
    return()
endif()

# Since we're not going through the normal pxr_build_test macros
# we need to elide tests here if requested.
if (NOT PXR_BUILD_TESTS)
    return()
endif()

# XXX:
# Disable tests in static builds due to symbol export issues.
if(WIN32 AND NOT TARGET shared_libs)
    return()
endif()

####
# Helper functions for unit tests
####

function(boost_python_cpp_target TARGET_NAME)
    set(options IS_LIBRARY)
    cmake_parse_arguments(opts "${options}" "" "" ${ARGN})

    if (opts_IS_LIBRARY)
        add_library(${TARGET_NAME} SHARED)
        set(destination "tests/lib")
    else()
        add_executable(${TARGET_NAME})
        set(destination "tests")
    endif()

    # If the monolithic shared library target exists the build will
    # fold our library into that target, so link against that instead.
    if (TARGET usd_ms)
        target_link_libraries(${TARGET_NAME} usd_ms)
    else()
        target_link_libraries(${TARGET_NAME} python)
    endif()

    target_include_directories(${TARGET_NAME}
        PRIVATE $<TARGET_PROPERTY:python,INCLUDE_DIRECTORIES>
        )

    # Setup RPATH so our python library under CMAKE_INSTALL_PREFIX is
    # found at runtime.
    pxr_set_rpaths_for_target(${TARGET_NAME}
        ORIGIN ${destination}
        RPATHS "${CMAKE_INSTALL_PREFIX}/lib"
    )
    
    install(
        TARGETS ${TARGET_NAME}
        DESTINATION ${destination}
    )
endfunction()

function(boost_python_test_module TEST_NAME)
    set(oneValueArgs MODULE_NAME)
    set(multiValueArgs MODULE_CPPFILES)
    cmake_parse_arguments(module "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT module_MODULE_NAME)
        set(module_MODULE_NAME "${TEST_NAME}_ext")
    endif()

    if (NOT module_MODULE_CPPFILES)
        set(module_MODULE_CPPFILES "test/${TEST_NAME}.cpp")
    endif()

    boost_python_cpp_target(${module_MODULE_NAME} IS_LIBRARY)
    target_sources(${module_MODULE_NAME} PRIVATE ${module_MODULE_CPPFILES})

    # Python modules must be suffixed with .pyd on Windows and .so on Linux/Mac
    if(WIN32)
        set(suffix ".pyd")
    else()
        set(suffix ".so")
    endif()

    set_target_properties(${module_MODULE_NAME}
        PROPERTIES
        # Python modules must not have any prefixes like "lib"
        PREFIX ""
        SUFFIX ${suffix}
    )
endfunction() # boost_python_test_module

function(boost_python_cpp_test_lib TEST_NAME)
    set(lib "${TEST_NAME}")
    set(cppfiles "test/${TEST_NAME}.cpp")

    boost_python_cpp_target(${lib} IS_LIBRARY)
    target_sources(${lib} PRIVATE ${cppfiles})
endfunction() # boost_python_cpp_test_lib

function(boost_python_test TEST_NAME)
    set(oneValueArgs SCRIPT)
    cmake_parse_arguments(test "" "${oneValueArgs}" "" ${ARGN})
    
    if (NOT test_SCRIPT)
        set(test_SCRIPT "test/${TEST_NAME}.py")
    endif()

    get_filename_component(installed_script "${test_SCRIPT}" NAME_WE)

    boost_python_test_module(${TEST_NAME} ${ARGN})
    pxr_test_scripts("${test_SCRIPT}")
    pxr_register_test("python_${TEST_NAME}"
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/pyrun ${installed_script}"
    )
endfunction() # boost_python_test

function(boost_python_cpp_test TEST_NAME)
    set(multiValueArgs CPPFILES COMMAND_ARGS)
    cmake_parse_arguments(test "" "" "${multiValueArgs}" ${ARGN})

    set(program "${TEST_NAME}")

    if (NOT test_CPPFILES)
        set(test_CPPFILES "test/${TEST_NAME}.cpp")
    endif()

    boost_python_cpp_target(${program})
    target_sources(${program} PRIVATE ${test_CPPFILES})

    pxr_register_test("python_${TEST_NAME}"
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/${program} ${test_COMMAND_ARGS}"
    )
endfunction() # boost_python_cpp_test

####
# Unit tests from boost::python
# These test definitions are ported from test/Jamfile
####

pxr_test_scripts(
  test/pyrun.py
)

boost_python_test("injected")
boost_python_test("properties")
boost_python_test("return_arg")
boost_python_test("staticmethod")

# boost::shared_ptr is unsupported in OpenUSD so we disable this test.
# boost_python_test("boost_shared_ptr")

boost_python_test("shared_ptr")
boost_python_test("enable_shared_from_this")
boost_python_test("andreas_beyer")
boost_python_test("wrapper_held_type")

boost_python_test("polymorphism")
boost_python_test("polymorphism2")
boost_python_test("minimal")
boost_python_test("args")
boost_python_test("raw_ctor")

boost_python_test("enum" 
    SCRIPT "test/test_enum.py"
    MODULE_CPPFILES "test/enum_ext.cpp"
)

boost_python_test("exception_translator")

boost_python_test("pearu1"
    SCRIPT "test/test_cltree.py"
    MODULE_NAME "cltree"
    MODULE_CPPFILES "test/cltree.cpp"
)

boost_python_test("try"
    SCRIPT "test/newtest.py"
    MODULE_NAME "m1"
    MODULE_CPPFILES "test/m1.cpp"
)
boost_python_test_module("try"
    MODULE_NAME "m2"
    MODULE_CPPFILES "test/m2.cpp"
)

boost_python_test("const_argument")

boost_python_test("keywords"
    SCRIPT "test/keywords_test.py"
    MODULE_NAME "keywords"
    MODULE_CPPFILES "test/keywords.cpp"
)

boost_python_test("builtin_converters"
    SCRIPT "test/test_builtin_converters.py"
    MODULE_CPPFILES "test/builtin_converters.cpp"
)

boost_python_test("test_pointer_adoption")
boost_python_test("operators")
boost_python_test("operators_wrapper")
boost_python_test("callbacks")
boost_python_test("defaults")
boost_python_test("object")
boost_python_test("class")
boost_python_test("aligned_class")
boost_python_test("list")
boost_python_test("long")
boost_python_test("dict")
boost_python_test("tuple")
boost_python_test("str")
boost_python_test("slice")
boost_python_test("virtual_functions")
boost_python_test("back_reference")
boost_python_test("implicit")
boost_python_test("data_members")
boost_python_test("ben_scott1")
boost_python_test("bienstman1")
boost_python_test("bienstman2")
boost_python_test("bienstman3")
boost_python_test("multi_arg_constructor")

boost_python_test("iterator")
boost_python_test_module("iterator"
    MODULE_NAME "input_iterator"
    MODULE_CPPFILES "test/input_iterator.cpp"
)

boost_python_test("stl_iterator")
boost_python_test("extract")

# XXX:
# On Windows the return_opaque_pointer return policy exercised by these
# tests causes an error when loading the Python module:
#
# DLL load failed: A dynamic link library (DLL) initialization routine failed.
#
# This appears to be a pre-existing issue in boost::python. These tests are
# disabled for now.

if (NOT WIN32)
    boost_python_test("crossmod_opaque"
        MODULE_NAME "crossmod_opaque_a"
        MODULE_CPPFILES "test/crossmod_opaque_a.cpp"
    )
    boost_python_test_module("crossmod_opaque"
        MODULE_NAME "crossmod_opaque_b"
        MODULE_CPPFILES "test/crossmod_opaque_b.cpp"
    )

    boost_python_test("opaque")
    boost_python_test("voidptr")
endif()

boost_python_test("pickle1")
boost_python_test("pickle2")
boost_python_test("pickle3")
boost_python_test("pickle4")
boost_python_test("nested")
boost_python_test("docstring")
boost_python_test("pytype_function")
boost_python_test("vector_indexing_suite")
boost_python_test("pointer_vector")

boost_python_test("map_indexing_suite"
    MODULE_CPPFILES
        "test/map_indexing_suite.cpp"
        "test/int_map_indexing_suite.cpp"
        "test/a_map_indexing_suite.cpp"
)

# These tests are disabled since we don't build the numpy extensions.
# boost_python_test("numpy/dtype")
# boost_python_test("numpy/ufunc")
# boost_python_test("numpy/templates")
# boost_python_test("numpy/ndarray")
# boost_python_test("numpy/indexing")
# boost_python_test("numpy/shapes")

boost_python_cpp_test("exec"
    COMMAND_ARGS "${CMAKE_INSTALL_PREFIX}/tests/exec.py"
)
install(
    FILES "test/exec.py"
    DESTINATION "tests"
)

boost_python_cpp_test("import_"
    COMMAND_ARGS "${CMAKE_INSTALL_PREFIX}/tests/import_.py"
)
install(
    FILES "test/import_.py"
    DESTINATION "tests"
)

boost_python_cpp_test_lib("indirect_traits_test")
boost_python_cpp_test("destroy_test")
boost_python_cpp_test("pointer_type_id_test")
boost_python_cpp_test("bases")
boost_python_cpp_test("if_else")
boost_python_cpp_test("pointee")
boost_python_cpp_test("result")
boost_python_cpp_test_lib("string_literal")
boost_python_cpp_test_lib("borrowed")
boost_python_cpp_test_lib("object_manager")
boost_python_cpp_test_lib("copy_ctor_mutates_rhs")
boost_python_cpp_test("upcast")
boost_python_cpp_test_lib("select_holder")

# select_from_python_test requires compiling in a source file from
# the library itself. However, we need to avoid this in monolithic
# builds. In that case, the library we built above will be an archive
# library that includes the associated object file; if we include it
# here, we'll run into duplicate symbol errors at link time.
#
# Because we're compiling in this extra source file, we need to set
# PXR_BOOST_PYTHON_STATIC_LIB to ensure the symbols from that file
# aren't set to be exported, which will cause conflicts with
# the header files that say they should be imported.
if (NOT PXR_BUILD_MONOLITHIC)
    set(extracppfiles "src/converter/type_id.cpp")
endif()

boost_python_cpp_test("select_from_python_test"
    CPPFILES
        "test/select_from_python_test.cpp"
        ${extracppfiles}
)
target_compile_definitions(select_from_python_test 
    PRIVATE PXR_BOOST_PYTHON_STATIC_LIB)

boost_python_cpp_test_lib("select_arg_to_python_test")
