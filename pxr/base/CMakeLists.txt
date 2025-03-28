set(DIRS
    arch
    tf
    gf
    pegtl
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

