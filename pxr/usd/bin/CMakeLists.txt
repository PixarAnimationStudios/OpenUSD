set(DIRS
    usdcat
    usdchecker
    usddiff
    usddumpcrate
    usdedit
    usdresolve
    usdstitch
    usdstitchclips
    usdzip
    sdfdump
    sdffilter
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

