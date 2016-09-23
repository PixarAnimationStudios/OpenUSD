if (NOT TARGET usdviewq)
    message(WARNING "Not building usdview because of missing dependency: usdviewq")
    return()
endif()

pxr_python_bins(
    usdview
)
