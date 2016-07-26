set(DIRS
    lib
#    bin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

