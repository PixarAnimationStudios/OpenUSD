#!/pxrpythonsubst

# Regression test for bug 101300. This bug was due to PcpLayerStack not
# recomputing its relocation tables when a prim that supplies 
# relocations was removed.

from pxr import Pcp, Sdf

import Mentor
from Mentor.Runtime import AssertEqual, FindDataFile

Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

rootLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug101300/root.sdf'))
pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

# Compute the prim index for /Root/A. The relocates authored on /Root/A
# should be applied, causing /Root/A/B to be renamed /Root/A/B_2.
(pi, err) = pcpCache.ComputePrimIndex('/Root/A')
AssertEqual(len(err), 0)
AssertEqual(pi.ComputePrimChildNames(), (['B_2'],['B']))
AssertEqual(pcpCache.layerStack.relocatesSourceToTarget,
            {Sdf.Path('/Root/A/B'): Sdf.Path('/Root/A/B_2')})

# Remove the over on /Root/A that provides the relocates. 
with Pcp._TestChangeProcessor(pcpCache):
    del rootLayer.GetPrimAtPath('/Root').nameChildren['A']

# The prim at /Root/A should still exist, but there should no longer be
# any relocates to apply.
(pi, err) = pcpCache.ComputePrimIndex('/Root/A')
AssertEqual(len(err), 0)
AssertEqual(pi.ComputePrimChildNames(), (['B'],[]))
AssertEqual(pcpCache.layerStack.relocatesSourceToTarget, {})

Mentor.Runtime.ExitTest()
