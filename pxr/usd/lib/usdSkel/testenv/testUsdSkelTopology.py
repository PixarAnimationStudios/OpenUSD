#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

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
            None # 9
        )

        expectedParentIndices = Vt.IntArray([-1, 2, 0, -1, -1, 4, 7, 8, -1, -1]) 

        topology = UsdSkel.Topology(targets)

        self.assertEqual(topology.GetNumJoints(), len(targets))
        self.assertEqual(topology.GetParentIndices(), expectedParentIndices)

        for i,parentIndex in enumerate(expectedParentIndices):
            self.assertEqual(parentIndex, topology.GetParent(i))


if __name__ == "__main__":
    unittest.main()
