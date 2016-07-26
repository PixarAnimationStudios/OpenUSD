set(DIRS
    lib
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
