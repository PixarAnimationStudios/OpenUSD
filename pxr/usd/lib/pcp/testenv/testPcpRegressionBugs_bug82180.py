#!/pxrpythonsubst

# Regression test for bug 82180. The ultimate cause of that bug was
# PcpLayerStack erroneously blowing its relocations when an insignificant
# change to the layer stack was being processed.

from pxr import Sdf, Pcp

import Mentor
from Mentor.Runtime import (FindDataFile, AssertTrue, AssertEqual)

Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

rootLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug82180/root.usda'))
sessionLayer = Sdf.Layer.CreateAnonymous()

pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer, sessionLayer))

# Compute the cache's root layer stack and verify its contents.
(layerStack, errors) = \
    pcpCache.ComputeLayerStack(pcpCache.GetLayerStackIdentifier())

AssertTrue(layerStack)
AssertEqual(layerStack.layers, [sessionLayer, rootLayer])
AssertEqual(layerStack.relocatesSourceToTarget, 
            { Sdf.Path('/ModelGroup/Model') : Sdf.Path('/ModelGroup/Model_1') })
AssertEqual(layerStack.relocatesTargetToSource, 
            { Sdf.Path('/ModelGroup/Model_1') : Sdf.Path('/ModelGroup/Model') })

AssertEqual(len(errors), 0)

# Now add an empty sublayer to the session layer, which should be regarded
# as an insignificant layer stack change.
#
# Note that this bug isn't specific to the session layer; it occurred with 
# any insignificant layer stack change. The test just uses the session layer
# since that's the repro steps from the bug report.
emptyLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug82180/empty.usda'))

with Pcp._TestChangeProcessor(pcpCache):
    sessionLayer.subLayerPaths.insert(0, emptyLayer.identifier)

# Verify that the layer stack's contents are still as expect; in particular,
# the relocations should be unaffected.
(layerStack, errors) = \
    pcpCache.ComputeLayerStack(pcpCache.GetLayerStackIdentifier())

AssertTrue(layerStack)
AssertEqual(layerStack.layers, [sessionLayer, emptyLayer, rootLayer])
AssertEqual(layerStack.relocatesSourceToTarget, 
            { Sdf.Path('/ModelGroup/Model') : Sdf.Path('/ModelGroup/Model_1') })
AssertEqual(layerStack.relocatesTargetToSource, 
            { Sdf.Path('/ModelGroup/Model_1') : Sdf.Path('/ModelGroup/Model') })

AssertEqual(len(errors), 0)

# All done!
Mentor.Runtime.ExitTest()
