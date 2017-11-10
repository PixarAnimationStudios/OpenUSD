#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from pxr import Gf

from maya import cmds
from maya.api import OpenMaya as OM
from maya import standalone

import os
import unittest
import pprint


class testUsdImportXforms(unittest.TestCase):

    EPSILON = 1e-6

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(cls):
        # Create a new file so each test case starts with a fresh state.
        cmds.file(new=1, f=1)        

    def _GetMayaTransform(self, transformName):
        selectionList = OM.MSelectionList()
        selectionList.add(transformName)
        mObj = selectionList.getDependNode(0)

        return OM.MFnTransform(mObj)

    def testImportInverseXformOpsOnly(self):
        """
        Tests that importing a USD cube mesh that has XformOps on it all tagged
        as inverse ops results in the correct transform when imported into Maya.
        """
        usdFile = os.path.abspath('UsdImportXformsTest.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

        mayaTransform = self._GetMayaTransform('InverseOpsOnlyCube')
        transformationMatrix = mayaTransform.transformation()

        expectedTranslation = [-1.0, -2.0, -3.0]
        actualTranslation = list(
            transformationMatrix.translation(OM.MSpace.kTransform))
        self.assertTrue(
            Gf.IsClose(expectedTranslation, actualTranslation, self.EPSILON))

        expectedRotation = [0.0, 0.0, Gf.DegreesToRadians(-45.0)]
        actualRotation = list(transformationMatrix.rotation())
        self.assertTrue(
            Gf.IsClose(expectedRotation, actualRotation, self.EPSILON))

        expectedScale = [2.0, 2.0, 2.0]
        actualScale = list(transformationMatrix.scale(OM.MSpace.kTransform))
        self.assertTrue(
            Gf.IsClose(expectedScale, actualScale, self.EPSILON))
        
    def testImportMayaXformVariations(self):
        """
        Tests that all combinations of the various maya xform pieces will
        import correctly
        """
        
        # create a bunch of transforms with varying transform attrs set
        from random import Random
        import itertools
        import os
        import pprint
        
        ATTRS = {
            'translate': (.01, 5),
            # we do rotates separately, so we can see "rotateY" and "rotateXYZ" ops
            'rotateX': (.01, 359.99),
            'rotateY': (.01, 359.99),
            'rotateZ': (.01, 359.99),
            'scale': (1.01, 2.0),
            'shear': (1.01, 2.0),
            'rotateOrder': (1, 5), 
            # it seems that internally rotateAxis is stored as a quaternion...
            # so to ensure proper roundtripping, keep values 0 < x < 90
            'rotateAxis': (.01, 89.99),
            'rotatePivot': (.01, 5),
            'scalePivot': (.01, 5),
            'rotatePivotTranslate': (.01, 5),
            'scalePivotTranslate': (.01, 5),
        }
        
        rand = Random(3)

        allNodes = []
        allExpected = {}
        
        topPrim = cmds.createNode('transform', name='topPrim')
        
        # Iterate through all combinations of whether each attr in ATTRS is set or not
        for i, enabledArray in enumerate(itertools.product((False, True), repeat=len(ATTRS))):
            # name will be like: mayaXform_000111010001
            node = 'mayaXform_{}'.format(''.join(str(int(x)) for x in enabledArray))
            node = cmds.createNode('transform', name=node, parent=topPrim)
            attrVals = {}
            allNodes.append(node)
            allExpected[node] = attrVals
            for enabled, (attr, (valMin, valMax)) in itertools.izip(enabledArray,
                                                                    ATTRS.iteritems()):
                if not enabled:
                    if attr in ('rotateOrder', 'rotateX', 'rotateY', 'rotateZ'):
                        attrVals[attr] = 0
                    elif attr == 'scale':
                        attrVals[attr] = (1, 1, 1)
                    else:
                        attrVals[attr] = (0, 0, 0)
                else:
                    if attr == 'rotateOrder':
                        # 1 - 5 because 0 (xyz) would correspond to "not enabled"
                        val = rand.randint(1, 5)
                    elif attr in ('rotateX', 'rotateY', 'rotateZ'):
                        val = rand.uniform(valMin, valMax)
                    else:
                        val = (rand.uniform(valMin, valMax),
                            rand.uniform(valMin, valMax),
                            rand.uniform(valMin, valMax))
                    attrVals[attr] = val
                    #node.setAttr(attr, val)
                    if isinstance(val, tuple):
                        cmds.setAttr("{}.{}".format(node, attr), *val)
                    else:
                        cmds.setAttr("{}.{}".format(node, attr), val)
        
        # Now write out a usd file with all our xforms...
        cmds.select(allNodes)
        usdPath = os.path.abspath('UsdImportMayaXformVariationsTest.usdc')
        cmds.usdExport(selection=1, file=usdPath)
        
        # Now import, and make sure it round-trips as expected
        cmds.file(new=1, f=1)
        cmds.usdImport(file=usdPath)
        for node, attrVals in allExpected.iteritems():
            # if only one (or less) of the three rotates is non-zero, then
            # the rotate order doesn't matter...
            nonZeroRotates = [attrVals['rotate' + dir] != 0 for dir in 'XYZ']
            skipRotateOrder = sum(int(x) for x in nonZeroRotates) <= 1 
            
            for attr, expectedVal in attrVals.iteritems():
                if attr == 'rotateOrder' and skipRotateOrder:
                    continue
                attrName = "{}.{}".format(node, attr)
                actualVal = cmds.getAttr(attrName)
                if not isinstance(expectedVal, tuple):
                    expectedVal = (expectedVal,)
                    actualVal = (actualVal,)
                else:
                    # cmds.getAttr('persp.scale') returns [(0, 0, 0)]... weird
                    actualVal = actualVal[0]
                for expected, actual in zip(expectedVal, actualVal):
                    try:
                        self.assertAlmostEqual(expected, actual,
                            msg="{} - expected {}, got {} (diff: {}".format(
                                attrName, expected, actual, abs(expected - actual)),
                            delta=1e-4)
                    except Exception:
                        print "full failed xform:"
                        pprint.pprint(attrVals)
                        raise


    def testPivot(self):
        """
        Tests that pivotPosition attribute doesn't interfere with the matrix
        that we get in maya when importing a usd file.
        """
        def _usdToMayaPath(usdPath):
            return str(usdPath).replace('/', '|')
        from maya import cmds
        cmds.loadPlugin('pxrUsd')
        usdFile = './pivotTests.usda'
        from pxr import Usd, UsdGeom
        stage = Usd.Stage.Open(usdFile)
        xformCache = UsdGeom.XformCache()

        cmds.usdImport(file=os.path.abspath(usdFile), primPath='/World')

        usdPaths = [
                '/World/anim/chars/SomeCharacter/Geom/Face/Eyes/LEye',
                '/World/anim/chars/SomeCharacter/Geom/Face/Eyes/LEye/Sclera_sbdv',
                '/World/anim/chars/SomeCharacter/Geom/Face/Eyes/REye/Sclera_sbdv',
                '/World/anim/chars/SomeCharacter/Geom/Hair/HairStandin/Hair/Hair_sbdv',
                '/World/anim/chars/SomeCharacter/Geom/Hair/HairStandin/Hair/HairFrontPiece_sbdv',
                ]

        for usdPath in usdPaths:
            usdMatrix = xformCache.GetLocalToWorldTransform(stage.GetPrimAtPath(usdPath))
            mayaPath = _usdToMayaPath(usdPath)
            mayaMatrix = Gf.Matrix4d(*cmds.xform(mayaPath, query=True, matrix=True, worldSpace=True))

            print 'testing matrix at', usdPath
            self.assertTrue(Gf.IsClose(
                usdMatrix.ExtractTranslation(), 
                mayaMatrix.ExtractTranslation(), 
                self.EPSILON))

if __name__ == '__main__':
    unittest.main(verbosity=2)
