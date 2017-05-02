set(DIRS
    garch
    hf
    cameraUtil
    pxOsd
    glf
    hd
    hdSt
    hdx
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

