set(DIRS
    usdcat
    usdcheck
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

