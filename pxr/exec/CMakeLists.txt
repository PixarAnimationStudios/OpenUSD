set(DIRS
    vdf
    ef
    esf
    esfUsd
    exec
    execUsd
    execGeom
)

# OpenExec currently supports hidden symbol visibility on Linux.
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
