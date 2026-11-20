set(DIRS
    execTest
    vdf
    ef
    esf
    esfUsd
    exec
    execUsd
    execGeom
    execIr
)

# OpenExec currently supports hidden symbol visibility on Linux, but not on
# Apple platforms. Note that CMAKE_CXX_VISIBILITY_PRESET has no effect if the
# compiler is MSVC.
#
# TODO: Hidden symbol visibility breaks the monolithic build on Linux. For now,
# we skip setting hidden symbol visibility in that case. This can be addressed
# and re-enabled in the future.
#
# With clang, ARCH_EXPORT_TYPE is type_visibility("default"), which does not
# export the members of the explicitly instantiated class templates (e.g.
# Vdf_VectorImplEmpty<T>), so the other libraries fail to link against them.
if(UNIX AND NOT APPLE AND NOT PXR_BUILD_MONOLITHIC AND
        NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
