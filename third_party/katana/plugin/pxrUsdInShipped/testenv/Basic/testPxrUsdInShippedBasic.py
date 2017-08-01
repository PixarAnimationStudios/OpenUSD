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
#
import unittest

from Katana import FnAttribute, KatanaFile, NodegraphAPI, Nodes3DAPI

model_blacklist = frozenset((
    'proxies.viewer.load.opArgs.a.fileName',
    'important',
    'pin',
    'prmanStatements'
))

def stripBlacklistAttrs(attrs, blacklist):
    """
    Strip blacklisted attributes from the given attributes. Examples
    of blacklisted attributes are internal-only attributes or ones that
    may change depending on the context in which this test is executed.
    """
    gb = FnAttribute.GroupBuilder()
    gb.update(attrs)

    for attrName in blacklist:
        gb.delete(attrName)

    return gb.build()

class TestPxrOpUsdInShippedBasic(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        KatanaFile.Load('basic.katana')

    def getBaselineAttrs(self, filename):
        with open(filename) as f:
            baselineAttrs = FnAttribute.Attribute.parseXML(f.read())
        return baselineAttrs

    def getLocationData(self, usdPath):
        client = Nodes3DAPI.CreateClient(NodegraphAPI.GetNode('PxrUsdIn'))
        return client.cookLocation('/root/world/geo%s' % usdPath)

    def test_mesh(self):
        attrs = self.getLocationData('/World/Mesh').getAttrs()
        baselineAttrs = self.getBaselineAttrs('mesh.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_xform(self):
        attrs = self.getLocationData('/World/Xform').getAttrs()
        baselineAttrs = self.getBaselineAttrs('xform.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_model(self):
        attrs = self.getLocationData('/World/anim/Model').getAttrs()
        attrs = stripBlacklistAttrs(attrs, model_blacklist)
        baselineAttrs = self.getBaselineAttrs('model.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_modelConstraints(self):
        locationData = self.getLocationData('/World/anim/Model')
        self.assertIn("ConstraintTargets", locationData.getPotentialChildren())
        
        locationData = self.getLocationData('/World/anim/Model/ConstraintTargets')
        self.assertIn("RootXf", locationData.getPotentialChildren())

    def test_camera(self):
        attrs = self.getLocationData('/World/main_cam').getAttrs()
        attrs = stripBlacklistAttrs(attrs, model_blacklist)
        baselineAttrs = self.getBaselineAttrs('camera.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_look(self):
        attrs = self.getLocationData('/World/Looks/PxrDisney5SG').getAttrs()
        baselineAttrs = self.getBaselineAttrs('look.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_faceSet(self):
        attrs = self.getLocationData('/World/FaceSet/Plane').getAttrs()
        baselineAttrs = self.getBaselineAttrs('plane.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

        attrs = self.getLocationData('/World/FaceSet/Plane/faceset_0').getAttrs()
        baselineAttrs = self.getBaselineAttrs('faceset0.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

        attrs = self.getLocationData('/World/FaceSet/Plane/faceset_1').getAttrs()
        baselineAttrs = self.getBaselineAttrs('faceset1.attrs')

        self.assertEqual(attrs.getHash(), baselineAttrs.getHash())

    def test_skippedBindingToLookNotUnderLooks(self):
        attrs = self.getLocationData('/World/Xform').getAttrs()
        self.assertTrue(attrs.getChildByName("materialAssign") is None)

if __name__ == '__main__':

    import sys

    # create test suite
    test_suite = unittest.TestSuite()
    test_suite.addTest(unittest.makeSuite(TestPxrOpUsdInShippedBasic))

    # run test suite
    runner = unittest.TextTestRunner()
    result = runner.run(test_suite)

    # verify result
    sys.exit(not result.wasSuccessful())
