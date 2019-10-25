set(DIRS
    arch
    tf
    gf
    js
    trace
    work
    plug
    vt
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()