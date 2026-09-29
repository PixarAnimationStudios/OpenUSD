set(DIRS
    usdauthors
    usdfixbrokenpixarschemas
    usdzip
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (PXR_BUILD_PMC_PLUGIN)
    add_subdirectory(usdcrush)
endif()

