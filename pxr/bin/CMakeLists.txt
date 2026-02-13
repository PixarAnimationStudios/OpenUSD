set(DIRS
    usdfixbrokenpixarschemas
    usdzip
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

