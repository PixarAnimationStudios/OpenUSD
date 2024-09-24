set(PXR_PREFIX performance)
set(PXR_PACKAGE usdviewq)

pxr_python_bin(usdmeasureperformance
    DEPENDENCIES
        usdviewq
)

pxr_library(usdperformance
    PYMODULE_FILES
        __init__.py
        parseTimingOutput.py
        explicitMetrics/stageTraversalMetric.py
)
