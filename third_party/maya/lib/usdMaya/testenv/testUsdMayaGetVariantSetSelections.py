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

import os
import unittest

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

from maya import cmds
from maya import standalone


class testUsdMayaGetVariantSetSelections(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        cls.usdFile = os.path.abspath('CubeWithVariantsModel.usda')
        cls.primPath = '/CubeWithVariantsModel'

    def setUp(self):
        cmds.file(new=True, force=True)

        self.assemblyNodeName = cmds.assembly(name='TestAssemblyNode',
            type='pxrUsdReferenceAssembly')
        cmds.setAttr('%s.filePath' % self.assemblyNodeName, self.usdFile, type='string')
        cmds.setAttr('%s.primPath' % self.assemblyNodeName, self.primPath, type='string')

    def _SetSelection(self, variantSetName, variantSelection):
        attrName = 'usdVariantSet_%s' % variantSetName

        if not cmds.attributeQuery(attrName, node=self.assemblyNodeName, exists=True):
            cmds.addAttr(self.assemblyNodeName, ln=attrName, dt='string', internalSet=True)

        cmds.setAttr('%s.%s' % (self.assemblyNodeName, attrName), variantSelection, type='string')

    def testNoSelections(self):
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {})

    def testOneSelection(self):
        self._SetSelection('modelingVariant', 'ModVariantB')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {'modelingVariant': 'ModVariantB'})

    def testAllSelections(self):
        self._SetSelection('fooVariant', 'FooVariantC')
        self._SetSelection('modelingVariant', 'ModVariantB')
        self._SetSelection('shadingVariant', 'ShadVariantA')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections,
            {'fooVariant': 'FooVariantC',
             'modelingVariant': 'ModVariantB',
             'shadingVariant': 'ShadVariantA'})

    def testBogusVariantName(self):
        self._SetSelection('bogusVariant', 'NotARealVariantSet')

        # Invalid variantSet names should not appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {})

    def testBogusSelection(self):
        self._SetSelection('modelingVariant', 'BogusSelection')

        # Selections are NOT validated, so any "selection" for a valid
        # variantSet should appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {'modelingVariant': 'BogusSelection'})


if __name__ == '__main__':
    unittest.main(verbosity=2)
