set(DIRS
    hdStream
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

