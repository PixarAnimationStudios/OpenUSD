set(DIRS
    usdcat
    usdcheck
    usddiff
    usdedit
    usdstitch
    usdstitchclips
    sdfdump
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

