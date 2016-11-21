set(DIRS
    garch
    hf
    cameraUtil
    pxOsd
    glf
    gal
    hd
    hdx
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

