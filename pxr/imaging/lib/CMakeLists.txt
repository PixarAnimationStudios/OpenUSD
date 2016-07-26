set(DIRS
    garch
    cameraUtil
    glf
    pxOsd
    hd
    glfq
    hdx
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

