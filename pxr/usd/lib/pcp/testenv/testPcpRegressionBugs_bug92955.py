#!/pxrpythonsubst

# Regression test for bug 92955.  Removing a class should not cause
# unrelated spooky sites to be flushed.

from pxr import Pcp, Sdf

import Mentor
from Mentor.Runtime import Assert, AssertEqual, AssertNotEqual, FindDataFile

Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

rootLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug92955/root.usda'))
pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

# Populate all prims in the scene
paths = [Sdf.Path('/')]
while paths:
    path, paths = paths[0], paths[1:]
    primIndex, _ = pcpCache.ComputePrimIndex(path)
    for name in primIndex.ComputePrimChildNames()[0]:
        paths.append(path.AppendChild(name))

# Verify there are symmetric corresponding paths on SymRig.
layerStack = pcpCache.layerStack
x = pcpCache.GetPathsUsingSite(layerStack, '/Model/Rig/SymRig', Pcp.Direct)
AssertNotEqual(len(x), 0)

# Delete the unrelated class.
with Pcp._TestChangeProcessor(pcpCache):
    del rootLayer.GetPrimAtPath('/Model/Rig').nameChildren['SymScope']

# Verify the same symmetric corresponding paths are on SymRig.
y = pcpCache.GetPathsUsingSite(layerStack, '/Model/Rig/SymRig', Pcp.Direct)
AssertEqual(x, y)
