set(DIRS
    usdcat
    usdcheck
    usddiff
    usdedit
    usdstitch
    usdstitchclips
    sdfdump
    sdffilter
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

