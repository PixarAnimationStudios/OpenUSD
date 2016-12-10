set(DIRS
    lib
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
