#!/pxrpythonsubst

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

import os

from pxr import Sdf
from pxr import Usd

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import AssertFalse
from Mentor.Runtime import AssertTrue
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUserExportedAttributes(Fixture):

    COMMON_ATTR_NAME = 'transformAndShapeAttr'

    def _GetExportedStage(self, mergeTransformAndShape=True):
        from maya import cmds

        cmds.file(FindDataFile('UserExportedAttributesTest.ma'), open=True, force=True)

        usdFilePathFormat = 'UserExportedAttributesTest_EXPORTED_%s.usda'
        if mergeTransformAndShape:
            usdFilePath = usdFilePathFormat % 'MERGED'
        else:
            usdFilePath = usdFilePathFormat % 'UNMERGED'
        usdFilePath = os.path.abspath(usdFilePath)

        # Export to USD.
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=mergeTransformAndShape,
            file=usdFilePath)

        stage = Usd.Stage.Open(usdFilePath)
        Assert(stage)

        return stage

    def TestExportAttributes(self):
        stage = self._GetExportedStage()

        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube')
        Assert(prim)

        exportedAttrs = {
            'realAttrOne':
                {'value': 42,
                 'typeName': Sdf.ValueTypeNames.Int},
            'my:namespace:realAttrTwo':
                {'value': 3.14,
                 'typeName': Sdf.ValueTypeNames.Double},
            'my:namespace:someNewAttr':
                {'value': 'a string value',
                 'typeName': Sdf.ValueTypeNames.String},
        }

        for attrName in exportedAttrs:
            attr = prim.GetAttribute(attrName)
            Assert(attr)
            AssertEqual(attr.Get(), exportedAttrs[attrName]['value'])
            AssertEqual(attr.GetTypeName(), exportedAttrs[attrName]['typeName'])
            AssertTrue(attr.IsCustom())

        # These attributes are all specified in the JSON but do not exist on
        # the Maya node, so we do NOT expect them to be exported.
        bogusAttrNames = ['bogusAttrOne', 'bogusAttrTwo', 'bogusRemapAttr',
            'my:namespace:bogusAttrTwo', 'my:namespace:someNewBogusAttr']

        for bogusAttrName in bogusAttrNames:
            attr = prim.GetAttribute(bogusAttrName)
            AssertFalse(attr.IsValid())

        # An attribute that has multiple tags all specifying the same USD
        # attribute name should result in the first Maya attribute name
        # lexicographically taking precedence.
        attr = prim.GetAttribute('multiplyTaggedAttr')
        Assert(attr)
        AssertEqual(attr.Get(), 20)

        # Since this test is merging transform and shape nodes, the value on
        # the USD prim for an attribute that is tagged on BOTH Maya nodes
        # should end up coming from the shape node.
        commonAttr = prim.GetAttribute(testUserExportedAttributes.COMMON_ATTR_NAME)
        Assert(commonAttr)
        AssertEqual(commonAttr.Get(), 'this node is a mesh')

    def TestExportAttributesUnmerged(self):
        stage = self._GetExportedStage(mergeTransformAndShape=False)

        # Since this test is NOT merging transform and shape nodes, there
        # should be a USD prim for each node, and they should have distinct
        # values for similarly named tagged attributes.
        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube')
        Assert(prim)
        commonAttr = prim.GetAttribute(testUserExportedAttributes.COMMON_ATTR_NAME)
        Assert(commonAttr)
        AssertEqual(commonAttr.Get(), 'this node is a transform')

        prim = stage.GetPrimAtPath('/UserExportedAttributesTest/Geom/Cube/CubeShape')
        Assert(prim)
        commonAttr = prim.GetAttribute(testUserExportedAttributes.COMMON_ATTR_NAME)
        Assert(commonAttr)
        AssertEqual(commonAttr.Get(), 'this node is a mesh')


if __name__ == '__main__':
    Runner().Main()
