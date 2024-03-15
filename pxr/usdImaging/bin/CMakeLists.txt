set(DIRS
    usdrecord
    usdview
    testusdview
    hdcapture
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (${PXR_ENABLE_MATERIALX_SUPPORT})
    add_subdirectory(usdBakeMtlx)
endif()
