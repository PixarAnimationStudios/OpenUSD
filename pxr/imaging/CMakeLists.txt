set(DIRS
    garch
    hf
    hio
    cameraUtil
    pxOsd
    glf
    hgi
    hgiGL
    hgiMetal
    hgiInterop
    hd
    hdSt
    hdx
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
