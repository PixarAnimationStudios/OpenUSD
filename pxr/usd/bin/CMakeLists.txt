set(DIRS
    usdcat
    usdcheck
    usddiff
    usdedit
    usdstitch
    usdstitchclips
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

