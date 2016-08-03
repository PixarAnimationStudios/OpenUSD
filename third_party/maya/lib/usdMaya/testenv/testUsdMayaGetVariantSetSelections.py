#!/bedrock_subst/bin/pypixb

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

import os

from pixar import UsdMaya

from maya import cmds

from Mentor.Runtime import AssertEqual
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testGetVariantSetSelections(Fixture):

    def ClassSetup(self):
        cmds.loadPlugin('pxrUsd')

        self.usdFile = os.path.abspath(FindDataFile('CubeWithVariantsModel.usda'))
        self.primPath = '/CubeWithVariantsModel'

    def Setup(self):
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

    def TestNoSelections(self):
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        AssertEqual(variantSetSelections, {})

    def TestOneSelection(self):
        self._SetSelection('modelingVariant', 'ModVariantB')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        AssertEqual(variantSetSelections, {'modelingVariant': 'ModVariantB'})

    def TestAllSelections(self):
        self._SetSelection('fooVariant', 'FooVariantC')
        self._SetSelection('modelingVariant', 'ModVariantB')
        self._SetSelection('shadingVariant', 'ShadVariantA')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        AssertEqual(variantSetSelections,
            {'fooVariant': 'FooVariantC',
             'modelingVariant': 'ModVariantB',
             'shadingVariant': 'ShadVariantA'})

    def TestBogusVariantName(self):
        self._SetSelection('bogusVariant', 'NotARealVariantSet')

        # Invalid variantSet names should not appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        AssertEqual(variantSetSelections, {})

    def TestBogusSelection(self):
        self._SetSelection('modelingVariant', 'BogusSelection')

        # Selections are NOT validated, so any "selection" for a valid
        # variantSet should appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        AssertEqual(variantSetSelections, {'modelingVariant': 'BogusSelection'})


if __name__ == '__main__':
    Runner().Main()
