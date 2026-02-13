# hdParticleField Render Delegate
hdParticleField is a sample render delegate implementing rendering of the
"particleField" prim type. It was designed to be a reference for gaussian
splat rendering, to assist in schema development, but does not currently
support other geometry types. It outputs a color/depth/primId tuple to support
compositing into the usdview viewport and picking/selection highlighting.

## Using hdParticleField
hdParticleField is a render delegate that is registered as a plugin. It can be
used as a viewport renderer (e.g. in usdview) through renderer discovery,
or it can be instantiated directly. 

Since hdParticleField lives in the extras folder, if you want to use it in
usdview, you will need to tell the plugin system to load hdParticleField
explicitly.

    setenv PXR_PLUGINPATH_NAME $PXR_PLUGINPATH_NAME:<inst>/share/usd/examples/plugin/hdParticleField/resources
