set(DIRS 
    usdVolImaging)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
