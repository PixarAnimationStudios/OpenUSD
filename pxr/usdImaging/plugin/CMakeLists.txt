set(DIRS 
    usdVolImaging
    usdShaders)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
