set(DIRS
    usdchecker
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
