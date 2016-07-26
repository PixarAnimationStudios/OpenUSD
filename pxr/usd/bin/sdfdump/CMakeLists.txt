pxr_cpp_bin(sdfdump
    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
 
    LIBRARIES
        arch
        plug
        tf
        sdf
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
