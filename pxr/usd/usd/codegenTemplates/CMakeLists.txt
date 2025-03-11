set(PXR_PREFIX {{ pxrPrefix }})
set(PXR_PACKAGE {{ libraryName }})

pxr_library({{ libraryName }}
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        # List dependency libraries

    CPPFILES
        # List non-schema cpp files

    PYMODULE_FILES
        __init__.py
)