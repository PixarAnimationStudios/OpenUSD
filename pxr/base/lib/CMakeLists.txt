set(DIRS
    arch
    tf
    gf
    js
    tracelite
    work
    plug
    vt
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()