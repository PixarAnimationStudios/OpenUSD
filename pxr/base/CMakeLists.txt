set(DIRS
    arch
    tf
    gf
    js
    trace
    work
    plug
    vt
#    bin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

