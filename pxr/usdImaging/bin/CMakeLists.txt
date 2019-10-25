set(DIRS
    usdrecord
    usdview
    testusdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
