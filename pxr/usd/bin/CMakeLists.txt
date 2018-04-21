set(DIRS
    usdcat
    usdcheck
    usddiff
    usddumpcrate
    usdedit
    usdstitch
    usdstitchclips
    sdfdump
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

