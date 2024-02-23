set(DIRS
    arch
    tf
    gf
    js
    trace
    work
    plug
    vt
    ts
#    bin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

