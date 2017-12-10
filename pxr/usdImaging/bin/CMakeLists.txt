set(DIRS
    usdview
    testusdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
