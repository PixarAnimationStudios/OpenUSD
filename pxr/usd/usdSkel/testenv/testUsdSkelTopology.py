#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdSkel, Vt, Sdf
import unittest


def MakeTargets(*paths):
    return [Sdf.Path(p) if p else Sdf.Path() for p in paths]


class TestUsdSkelTopology(unittest.TestCase):

    def test_Topology(self):
        """Tests basics of a UsdSkelTopology"""

        targets = MakeTargets(
            # Basic parenting. Order shouldn't matter.
            "A", # 0
            "A/B/C", # 1
            "A/B", # 2
            "Nested/Without/Parent/Joint", # 2
            # The direct parent path of a joint may not
            # be included. Will use the first valid ancestor
            # path that is in the set as the parent.
            "D", # 4
            "D/E/F/G", # 5
            # Abs. vs. relative paths shouldn't matter,
            # But we won't map relative paths to abs paths, or vice versa.
            "/A/B/C", # 6
            "/A/B", # 7
            "/A", # 8
            # empty paths should be treated as roots.
            None, # 9,
            # Test invalid paths.
            "/", ".."
        )

        expectedParentIndices = Vt.IntArray(
            [-1, 2, 0, -1, -1, 4, 7, 8, -1, -1, -1, -1]) 

        topology = UsdSkel.Topology(targets)

        self.assertEqual(topology.GetNumJoints(), len(targets))
        self.assertEqual(topology.GetParentIndices(), expectedParentIndices)

        for i,parentIndex in enumerate(expectedParentIndices):
            self.assertEqual(parentIndex, topology.GetParent(i))
            if parentIndex < 0:
                self.assertTrue(topology.IsRoot(i))


    def test_ValidateTopology(self):
        """Test UsdSkelTopology validation"""

        topology = UsdSkel.Topology(
            MakeTargets("A", "A/B", "A/B/C", "D", "D/E"))
        self.assertTrue(topology)
        valid,reason = topology.Validate()
        self.assertTrue(valid)
        
        # Mis-orderered topology (parents don't come before children)
        topology = UsdSkel.Topology(MakeTargets("A/B", "C", "A"))
        self.assertTrue(topology)
        valid,reason = topology.Validate()
        self.assertFalse(valid)
        self.assertTrue(reason)


if __name__ == "__main__":
    unittest.main()
