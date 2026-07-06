set(PXR_PACKAGE invertibleRigsExampleUsdviewPlugin)

pxr_plugin(${PXR_PACKAGE}
    PYMODULE_FILES
        __init__.py
        plugin.py
        plugInfo.json
)