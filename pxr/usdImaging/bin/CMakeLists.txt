set(DIRS
    usdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
