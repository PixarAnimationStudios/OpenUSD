set(DIRS
    usdcat
    usdcheck
    usdchecker
    usddiff
    usdedit
    usdstitch
    usdstitchclips
    usdzip
    sdfdump
    sdffilter
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

