#!/pxrpythonsubst

from pxr import Sdf, Pcp, Tf

from Mentor.Runtime import (AssertEqual, AssertTrue, FindDataFile, 
                            ExpectedErrors, RequiredException)

# Create a PcpCache for a reference chain, but do not perform any actual
# composition before querying its used layers. Verify that this does not crash.
file = FindDataFile('testPcpMuseum.testenv/BasicReference/root.usda')
assert file
layer = Sdf.Layer.FindOrOpen(file)
assert layer
lsi = Pcp.LayerStackIdentifier(layer)
assert lsi
pcpCache = Pcp.Cache(lsi)
assert pcpCache
pcpCache.GetUsedLayers()

# Create a PcpCache with a target schema, ensuring that layers
# without the correct target will be marked invalid during composition.
#
# XXX: Need to divorce Pcp from menva.
pcpCache = Pcp.Cache(lsi, targetSchema='Presto')
(pi, _) = pcpCache.ComputePrimIndex('/PrimWithReferences')
AssertEqual(len(pi.localErrors), 0)

# XXX: Sdf currently emits coding errors when it cannot find a file format
#      for the given target. I'm not sure this should be the case; it
#      probably should treat it as though the file didn't exist, which
#      does not emit errors.
pcpCache = Pcp.Cache(lsi, targetSchema='sdf')

with ExpectedErrors(2):
    with RequiredException(Tf.ErrorException):
        pcpCache.ComputePrimIndex('/PrimWithReferences')

# Should be two local errors corresponding to invalid asset paths,
# since this prim has two references to layers with a different target
# schema.
(pi, _) = pcpCache.ComputePrimIndex('/PrimWithReferences')
AssertEqual(len(pi.localErrors), 2)
AssertTrue(all([(isinstance(e, Pcp.ErrorInvalidAssetPath) 
                 for e in pi.localErrors)]))


def TestPcpCacheReloadSessionLayers():

    rootLayer = Sdf.Layer.CreateAnonymous()

    sessionRootLayer = Sdf.Layer.CreateAnonymous()
    assert sessionRootLayer
    sessionSubLayer = Sdf.Layer.CreateAnonymous()
    assert sessionSubLayer

    sessionRootLayer.subLayerPaths.append(sessionSubLayer.identifier)

    # Author something to the sublayer
    primSpec = Sdf.PrimSpec(sessionSubLayer, "Root", Sdf.SpecifierDef)
    assert sessionSubLayer.GetPrimAtPath("/Root")

    # Create the Pcp structures
    lsi = Pcp.LayerStackIdentifier(rootLayer, sessionRootLayer)
    assert lsi
    pcpCache = Pcp.Cache(lsi)
    assert pcpCache
    pcpCache.ComputeLayerStack(lsi)

    # Now reload and make sure that the spec on the sublayer stays intact
    pcpCache.Reload()
    assert sessionSubLayer.GetPrimAtPath("/Root")

TestPcpCacheReloadSessionLayers()
