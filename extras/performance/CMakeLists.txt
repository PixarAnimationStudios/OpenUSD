if (NOT PXR_BUILD_USDVIEW)
    message(STATUS "Not installing performance scripts since usdview is "
                   "not available")
    return()
endif()

set(PXR_PREFIX performance)
set(PXR_PACKAGE extras_performance)

pxr_python_bin(usdmeasureperformance)

pxr_library(Performance
    PYMODULE_FILES
        __init__.py
        parseTimingOutput.py
        ExplicitMetrics/stageTraversalMetric.py
)
