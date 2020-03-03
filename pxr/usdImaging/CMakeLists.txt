set(DIRS
    lib
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
