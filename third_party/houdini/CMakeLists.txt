if (APPLE)   
    message(WARNING "Skipping third_party/houdini because the Houdini plugin is not supported on MacOS")
    return()
elif (WIN32)
    message(WARNING "Skipping third_party/houdini because the Houdini plugin is not supported on Windows")
    return()
endif()

set(PXR_INSTALL_SUBDIR "third_party/houdini")

pxr_setup_plugins()

add_definitions( 
    -DHOUDINI_API_VERSION=${HOUDINI_MAJOR_VERSION}${HOUDINI_MINOR_VERSION}
    -DVERSION="${HOUDINI_VERSION}"
    -DHOUDINI_DOT_VERSION=${HOUDINI_BUILD_VERSION} )

add_definitions(  -fpermissive -D_GNU_SOURCE -DLINUX -DAMD64 -DSIZEOF_VOID_P=8 -DFBX_ENABLED=1 -DOPENCL_ENABLED=1 -DOPENVDB_ENABLED=1 
-DMAKING_DSO -DSESI_LITTLE_ENDIAN -DENABLE_THREADS -DUSE_PTHREADS -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DGCC4 -DGCC3 
-Wall -Wno-parentheses -Wno-sign-compare -Wno-reorder -Wno-uninitialized -Wunused -Wno-unused-parameter -Wno-unused-local-typedefs -Wno-empty-body -fno-strict-aliasing )

add_subdirectory(lib)
add_subdirectory(plugin)
