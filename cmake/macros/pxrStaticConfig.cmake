# Make sure the plugins include all symbols when building with emscripten
set(CMAKE_CXX_LINK_LIBRARY_USING_LOAD_PLUGIN_SUPPORTED ON)
set(CMAKE_CXX_LINK_LIBRARY_USING_LOAD_PLUGIN
    "-Wl,--whole-archive"
    "<LINK_ITEM>"
    "-Wl,--no-whole-archive"
    )
