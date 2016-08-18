#!/pxrpythonsubst

# Regression test case for bug 70951. Adding inert prim specs was not
# properly updating the 'hasSpecs' field on the associated node in the
# prim index.

from pxr import Sdf, Pcp
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, ExitTest,
                            FindDataFile, AssertTrue, AssertFalse, AssertEqual)

rootLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug70951/root.usda'))
refLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug70951/JoyGroup.usda'))

pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

# Compute the prim index for the Sim scope under JoyGroup. This index
# should consist of two nodes: the direct node and the reference node.
# Since there is no spec for the Sim scope in the root layer, the root node
# should be marked as such.
(primIndex, errors) = \
    pcpCache.ComputePrimIndex('/World/anim/chars/JoyGroup/Sim')
AssertEqual(len(errors), 0)
AssertEqual(primIndex.primStack, [refLayer.GetPrimAtPath('/JoyGroup/Sim')])
AssertFalse(primIndex.rootNode.hasSpecs)

# Now create an empty over for the Sim scope in the root layer and verify
# that the prim index sees this new spec after change processing.
with Pcp._TestChangeProcessor(pcpCache):
    emptyOver = \
        Sdf.CreatePrimInLayer(rootLayer, '/World/anim/chars/JoyGroup/Sim')
    AssertTrue(emptyOver.IsInert())

(primIndex, errors) = \
    pcpCache.ComputePrimIndex('/World/anim/chars/JoyGroup/Sim')
AssertEqual(len(errors), 0)
AssertEqual(primIndex.primStack, 
            [rootLayer.GetPrimAtPath('/World/anim/chars/JoyGroup/Sim'),
             refLayer.GetPrimAtPath('/JoyGroup/Sim')])
AssertTrue(primIndex.rootNode.hasSpecs)

# All is well!
ExitTest()
