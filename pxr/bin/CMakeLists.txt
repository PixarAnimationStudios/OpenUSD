set(DIRS
    usdfixbrokenpixarschemas
    usdzip
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_USDPMC)
    add_subdirectory(usdcrush)
endif()

