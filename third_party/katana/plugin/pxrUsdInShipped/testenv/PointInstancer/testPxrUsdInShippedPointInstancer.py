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
import filecmp
import os

from Katana import NodegraphAPI, CacheManager
from PyUtilModule import AttrDump

class TestPxrOpUsdInInternalPointInstancer(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        '''Procedurally create nodes for testing.'''

        # Add a PxrUsdIn node and set its motion sample times
        pxrUsdInNode = NodegraphAPI.CreateNode(
            'PxrUsdIn', NodegraphAPI.GetRootNode())
        pxrUsdInNode.getParameter('motionSampleTimes').setValue('0 -1', 0)

        # Add a RenderSettings node and turn on motion blur
        renderSettingsNode = NodegraphAPI.CreateNode(
            'RenderSettings', NodegraphAPI.GetRootNode())
        mts = renderSettingsNode.getParameter(
            'args.renderSettings.maxTimeSamples')
        mts.getChild('value').setValue(2, 0)
        mts.getChild('enable').setValue(1, 0)
        sc = renderSettingsNode.getParameter(
            'args.renderSettings.shutterClose')
        sc.getChild('value').setValue(0.5, 0)
        sc.getChild('enable').setValue(1, 0)

        # Connect the two nodes together and view from the RS node
        pxrUsdInNode.getOutputPortByIndex(0).connect(
            renderSettingsNode.getInputPortByIndex(0))
        NodegraphAPI.SetNodeViewed(renderSettingsNode, True, True)

        # Set the current frame
        NodegraphAPI.SetCurrentTime(50)

    def cleanUpTestFile(self, testfile):
        '''Prep the given test file for a baseline compare.'''
        cwd = os.getcwd() + '/'
        with open(testfile, 'r') as f:
            contents = f.read()

        # Remove the current working directory
        contents = contents.replace(cwd, '')

        # Remove irrelevant attrs by ignoring all dump results prior to those
        # for /root/world/geo
        locKey = '<location path='
        splitContents = contents.split(locKey)
        newContents = ""
        preserve = False
        for piece in splitContents:
            if piece.startswith('"/root/world/geo">'):
                preserve = True
            if preserve:
                newContents += (locKey + piece)

        with open(testfile, 'w') as f:
            f.write(newContents)

    def compareAgainstBaseline(self, testfile):
        '''Compare the given test file to its associated baseline file.'''
        baselinefile = testfile.replace('test.', 'baseline.')
        print 'Comparing %s against baseline %s' % \
                (os.path.abspath(testfile), os.path.abspath(baselinefile))
        if filecmp.cmp(testfile, baselinefile, shallow=False):
            return True
        return False

    def test_motion(self):
        '''Change the PxrUsdIn's file and verify the dumped result.'''

        # test.motion.usda is a UsdGeomPointInstancer with positions,
        # orientations, scales, velocities, and angular velocities
        NodegraphAPI.GetNode('PxrUsdIn').getParameter('fileName').setValue(
            'test.motion.usda', 0)
        CacheManager.flush()

        testfile = 'test.motion.xml'
        AttrDump.AttrDump(testfile)
        self.assertTrue(os.path.exists(testfile))

        self.cleanUpTestFile(testfile)
        self.assertTrue(self.compareAgainstBaseline(testfile))

    def test_translateOnly(self):
        '''Change the PxrUsdIn's file and verify the dumped result.'''

        # test.translateOnly.usda is a UsdGeomPointInstancer with positions
        NodegraphAPI.GetNode('PxrUsdIn').getParameter('fileName').setValue(
            'test.translateOnly.usda', 0)
        CacheManager.flush()

        testfile = 'test.translateOnly.xml'
        AttrDump.AttrDump(testfile)
        self.assertTrue(os.path.exists(testfile))

        self.cleanUpTestFile(testfile)
        self.assertTrue(self.compareAgainstBaseline(testfile))

if __name__ == '__main__':
    import sys

    test_suite = unittest.TestSuite()
    test_suite.addTest(unittest.makeSuite(TestPxrOpUsdInInternalPointInstancer))

    runner = unittest.TextTestRunner()
    result = runner.run(test_suite)

    sys.exit(not result.wasSuccessful())
