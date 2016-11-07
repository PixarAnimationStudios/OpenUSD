set(DIRS
    garch
    hf
    cameraUtil
    pxOsd
    glf

    hd
    hdx
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

