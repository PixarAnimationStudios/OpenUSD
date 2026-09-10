set(DIRS
    usdauthors
    usdfixbrokenpixarschemas
    usdzip
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

