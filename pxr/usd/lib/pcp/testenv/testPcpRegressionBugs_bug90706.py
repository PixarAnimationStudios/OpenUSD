#!/pxrpythonsubst

# Regression test for bug 90706. Adding a new prim spec
# within a variant that's being used by a prim index should
# cause that prim index to be updated.

from pxr import Sdf, Pcp

import Mentor
from Mentor.Runtime import AssertEqual, AssertFalse, AssertTrue, FindDataFile

Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

rootLayer = Sdf.Layer.FindOrOpen(
    FindDataFile('testPcpRegressionBugs.testenv/bug90706/root.sdf'))
pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

(index, err) = pcpCache.ComputePrimIndex('/Sarah/EmptyPrim')
AssertTrue(index)
AssertEqual(len(err), 0)

# This prim index should only contain the root node; the variant
# arc to /Sarah{displayColor=red}EmptyPrim will have been culled
# because there's no prim spec there.
AssertEqual(len(index.rootNode.children), 0)

with Pcp._TestChangeProcessor(pcpCache):
    p = Sdf.CreatePrimInLayer(
        rootLayer, '/Sarah{displayColor=red}EmptyPrim')

# The addition of the inert prim should have caused the prim index
# to be blown, because it needs to be re-computed to accommodate the
# no-longer-inert variant node.
AssertFalse(pcpCache.FindPrimIndex('/Sarah/EmptyPrim'))
(index, err) = pcpCache.ComputePrimIndex('/Sarah/EmptyPrim')
AssertTrue(index)
AssertEqual(len(err), 0)

# There should now be a variant arc for /Sarah{displayColor=red}EmptyPrim
# in the local layer stack.
AssertEqual(len(index.rootNode.children), 1)
AssertEqual(index.rootNode.children[0].arcType, 
            Pcp.ArcTypeVariant)
AssertEqual(index.rootNode.children[0].layerStack,
            index.rootNode.layerStack)
AssertEqual(index.rootNode.children[0].path, 
            '/Sarah{displayColor=red}EmptyPrim')

Mentor.Runtime.ExitTest()
