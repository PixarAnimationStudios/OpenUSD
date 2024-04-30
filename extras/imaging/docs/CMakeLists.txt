set(PXR_PREFIX pxr/extras/imaging)
set(PXR_PACKAGE extras_imaging_docs)

pxr_docs_only_dir(${PXR_PACKAGE}
    DOXYGEN_FILES
        hydra_getting_started_guide.dox
        hydra_scene_browser.dox
        hydra_prim_schemas.dox
        examples_hydra_getting_started.cpp
        app_renderer_filters.png
        hdschema.png
        legacy_scene_delegate_emulation.png
        nested_datasources.png
        scene_index_1.png
        scene_index_2.png
        scene_index_adapters.png
        scene_index_filter.png
        usdimagingstagesceneindex_filters.png
        hydra_scene_browser_usdview.png
        hydra_scene_browser_columns.png
        hydra_scene_browser_filters.png        
)