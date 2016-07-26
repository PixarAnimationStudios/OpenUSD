set(DIRS
    usdImaging
    usdviewq
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
