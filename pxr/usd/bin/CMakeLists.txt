set(DIRS
    usdcat
    usdchecker
    usddiff
    usdedit
    usdstitch
    usdstitchclips
    usdzip
    sdfdump
    sdffilter
    usdtree
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

