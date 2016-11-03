set(DIRS
    garch
    cameraUtil
    glf
    pxOsd
    hd
    hdx
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

