set(PXR_PACKAGE invertibleRigsExample)

pxr_docs_only_dir(${PXR_PACKAGE}
    DOXYGEN_FILES
        README.md
        docs/images/waddlerRig.drawio.svg
)

add_subdirectory(invertibleRigsExampleAuthoringCode)
add_subdirectory(invertibleRigsExampleUsdviewPlugin)
