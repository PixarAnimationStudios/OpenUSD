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
#

from pxr import UsdMaya

from pxr import Gf
from pxr import Usd

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

from collections import OrderedDict
import itertools
import os
import pprint
import unittest


class testUsdMayaXformStack(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
    
    # Named so this runs first, so we can be reasonably sure
    # that UsdGeom hasn't been imported yet - see below
    # tests:
    #   UsdMayaXformOpClassification::GetOpType
    def test00GetOpType(self):
        translateOp = UsdMaya.XformStack.CommonStack().GetOps()[0]
        # could import UsdGeom, and compare objects
        # directly, but had a bug where if you didn't
        # import UsdGeom, the wrappers for
        # XformOp::Type weren't created yet... so test
        # for this
        self.assertEqual(
            str(translateOp.GetOpType()),
            'UsdGeom.XformOp.TypeTranslate')
        # ok, we've tested the XformOp::Type wrapper,
        # may as well make sure the actual objects
        # line up
        from pxr import UsdGeom
        self.assertEqual(
            translateOp.GetOpType(),
            UsdGeom.XformOp.TypeTranslate)

    # tests:
    #   UsdMayaXformOpClassification::GetName
    #   UsdMayaXformOpClassification::GetOpType
    #   UsdMayaXformOpClassification::IsInvertedTwin
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::GetOps    
    def testMayaStack(self):
        from pxr import UsdGeom
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(
            [(x.GetName(), x.IsInvertedTwin(), x.GetOpType()) for x in mayaStack.GetOps()],
            [
                ('translate',             False, UsdGeom.XformOp.TypeTranslate),
                ('rotatePivotTranslate',  False, UsdGeom.XformOp.TypeTranslate),
                ('rotatePivot',           False, UsdGeom.XformOp.TypeTranslate),
                ('rotate',                False, UsdGeom.XformOp.TypeRotateXYZ),
                ('rotateAxis',            False, UsdGeom.XformOp.TypeRotateXYZ),
                ('rotatePivot',           True,  UsdGeom.XformOp.TypeTranslate),
                ('scalePivotTranslate',   False, UsdGeom.XformOp.TypeTranslate),
                ('scalePivot',            False, UsdGeom.XformOp.TypeTranslate),
                ('shear',                 False, UsdGeom.XformOp.TypeTransform),
                ('scale',                 False, UsdGeom.XformOp.TypeScale),
                ('scalePivot',            True,  UsdGeom.XformOp.TypeTranslate),
            ]
        )
            
    # tests:
    #   UsdMayaXformOpClassification::GetName
    #   UsdMayaXformOpClassification::GetOpType
    #   UsdMayaXformOpClassification::IsInvertedTwin
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformStack::GetOps
    def testCommonStack(self):
        from pxr import UsdGeom
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(
            [(x.GetName(), x.IsInvertedTwin(), x.GetOpType()) for x in commonStack.GetOps()],
            [
                ('translate', False, UsdGeom.XformOp.TypeTranslate),
                ('pivot',     False, UsdGeom.XformOp.TypeTranslate),
                ('rotate',    False, UsdGeom.XformOp.TypeRotateXYZ),
                ('scale',     False, UsdGeom.XformOp.TypeScale),
                ('pivot',     True,  UsdGeom.XformOp.TypeTranslate),
            ]
        )
        
    # tests:
    #   UsdMayaXformOpClassification::GetName
    #   UsdMayaXformOpClassification::GetOpType
    #   UsdMayaXformOpClassification::IsInvertedTwin
    #   UsdMayaXformStack::MatrixStack
    #   UsdMayaXformStack::GetOps
    def testMatrixStack(self):
        from pxr import UsdGeom
        matrixStack = UsdMaya.XformStack.MatrixStack()
        self.assertEqual(
            [(x.GetName(), x.IsInvertedTwin(), x.GetOpType()) for x in matrixStack.GetOps()],
            [('transform', False, UsdGeom.XformOp.TypeTransform)]
        )

    # tests:
    #   UsdMayaXformOpClassification::IsCompatibleType
    #   UsdMayaXformStack::CommonStack        
    #   UsdMayaXformStack::FindOp
    def testIsCompatibleType(self):
        from pxr import UsdGeom
        commonStack = UsdMaya.XformStack.CommonStack()
        
        translateOp = commonStack.FindOp('translate') 
        for opType in UsdGeom.XformOp.Type.allValues:
            if opType == UsdGeom.XformOp.TypeTranslate:
                self.assertTrue(translateOp.IsCompatibleType(opType),
                                "{} should be compatible with {}".format(
                                    translateOp.GetName(),
                                    opType))
            else:
                self.assertFalse(translateOp.IsCompatibleType(opType),
                                "{} should not be compatible with {}".format(
                                    translateOp.GetName(),
                                    opType))
                
        rotateOp = commonStack.FindOp('rotate')
        for opType in UsdGeom.XformOp.Type.allValues:
            if str(opType).startswith('UsdGeom.XformOp.TypeRotate'):
                self.assertTrue(rotateOp.IsCompatibleType(opType),
                                "{} should be compatible with {}".format(
                                    rotateOp.GetName(),
                                    opType))
            else:
                self.assertFalse(rotateOp.IsCompatibleType(opType),
                                "{} should not be compatible with {}".format(
                                    rotateOp.GetName(),
                                    opType))
    
    # tests:
    #   UsdMayaXformOpClassification::operator==
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformStack::MayaStack
    def testOpClassEqual(self):
        mayaTranslate = UsdMaya.XformStack.MayaStack().GetOps()[0]
        commonTranslate = UsdMaya.XformStack.CommonStack().GetOps()[0]
        self.assertIsNot(mayaTranslate, commonTranslate)
        self.assertEqual(mayaTranslate, commonTranslate)

    # tests:
    #   UsdMayaXformOpClassification::CompatibleAttrNames
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::FindOp
    def testCompatibleAttrNames(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        
        translateOp = mayaStack.FindOp('translate')
        self.assertItemsEqual(
            translateOp.CompatibleAttrNames(),
            [
                'xformOp:translate:translate',
                'xformOp:translate',  
            ]
        )
        
        rotateOp = mayaStack.FindOp('rotate')
        self.assertItemsEqual(
            rotateOp.CompatibleAttrNames(),
            [
                'xformOp:rotateX',
                'xformOp:rotateX:rotate',
                'xformOp:rotateX:rotateX',
                'xformOp:rotateY',
                'xformOp:rotateY:rotate',
                'xformOp:rotateY:rotateY',
                'xformOp:rotateZ',
                'xformOp:rotateZ:rotate',
                'xformOp:rotateZ:rotateZ',
                'xformOp:rotateXYZ',
                'xformOp:rotateXYZ:rotate',
                'xformOp:rotateXYZ:rotateXYZ',
                'xformOp:rotateXZY',
                'xformOp:rotateXZY:rotate',
                'xformOp:rotateXZY:rotateXZY',
                'xformOp:rotateYXZ',
                'xformOp:rotateYXZ:rotate',
                'xformOp:rotateYXZ:rotateYXZ',
                'xformOp:rotateYZX',
                'xformOp:rotateYZX:rotate',
                'xformOp:rotateYZX:rotateYZX',
                'xformOp:rotateZXY',
                'xformOp:rotateZXY:rotate',
                'xformOp:rotateZXY:rotateZXY',
                'xformOp:rotateZYX',
                'xformOp:rotateZYX:rotate',
                'xformOp:rotateZYX:rotateZYX',
            ]
        )
        
        shearOp = mayaStack.FindOp('shear')
        self.assertItemsEqual(
            shearOp.CompatibleAttrNames(),
            [
                'xformOp:transform:shear',
            ]
        )
        
        rotatePivotOp = mayaStack.FindOp('rotatePivot', isInvertedTwin=True)
        self.assertItemsEqual(
            rotatePivotOp.CompatibleAttrNames(),
            [
                'xformOp:translate:rotatePivot',
            ]
        )
        
    def testNoInit(self):
        try:
            UsdMaya.XformOpClassification()
        except Exception as e:
            self.assertTrue('cannot be instantiated from Python' in str(e))

        try:
            UsdMaya.XformStack()
        except Exception as e:
            self.assertTrue('cannot be instantiated from Python' in str(e))

    # tests:
    #   UsdMayaXformStack::GetInversionTwins
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformStack::MatrixStack
    def testGetInversionTwins(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(mayaStack.GetInversionTwins(),
                         [(2, 5), (7, 10)])
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(commonStack.GetInversionTwins(),
                         [(1, 4)])
        matrixStack = UsdMaya.XformStack.MatrixStack()
        self.assertEqual(matrixStack.GetInversionTwins(),
                         [])
    
    # tests:
    #   UsdMayaXformStack::GetNameMatters
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformStack::MatrixStack
    def testGetNameMatters(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertTrue(mayaStack.GetNameMatters())
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertTrue(commonStack.GetNameMatters())
        matrixStack = UsdMaya.XformStack.MatrixStack()
        self.assertFalse(matrixStack.GetNameMatters())
        
    # tests:
    #   UsdMayaXformStack::GetSize
    #   UsdMayaXformStack::(__len__)
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformStack::MatrixStack
    def testGetSize(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(len(mayaStack), 11)
        self.assertEqual(mayaStack.GetSize(), 11)
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(len(commonStack), 5)
        self.assertEqual(commonStack.GetSize(), 5)
        matrixStack = UsdMaya.XformStack.MatrixStack()
        self.assertEqual(len(matrixStack), 1)
        self.assertEqual(matrixStack.GetSize(), 1)
        
    #   UsdMayaXformStack::operator[]
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformOpClassification::GetName
    def testIndexing(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(mayaStack[0].GetName(), 'translate')
        self.assertEqual(mayaStack[1].GetName(), 'rotatePivotTranslate')
        self.assertEqual(mayaStack[2].GetName(), 'rotatePivot')
        self.assertEqual(mayaStack[3].GetName(), 'rotate')
        self.assertEqual(mayaStack[4].GetName(), 'rotateAxis')
        self.assertEqual(mayaStack[5].GetName(), 'rotatePivot')
        self.assertEqual(mayaStack[6].GetName(), 'scalePivotTranslate')
        self.assertEqual(mayaStack[7].GetName(), 'scalePivot')
        self.assertEqual(mayaStack[8].GetName(), 'shear')
        self.assertEqual(mayaStack[9].GetName(), 'scale')
        self.assertEqual(mayaStack[10].GetName(), 'scalePivot')

        self.assertEqual(mayaStack[-11].GetName(), 'translate')
        self.assertEqual(mayaStack[-10].GetName(), 'rotatePivotTranslate')
        self.assertEqual(mayaStack[-9].GetName(), 'rotatePivot')
        self.assertEqual(mayaStack[-8].GetName(), 'rotate')
        self.assertEqual(mayaStack[-7].GetName(), 'rotateAxis')
        self.assertEqual(mayaStack[-6].GetName(), 'rotatePivot')
        self.assertEqual(mayaStack[-5].GetName(), 'scalePivotTranslate')
        self.assertEqual(mayaStack[-4].GetName(), 'scalePivot')
        self.assertEqual(mayaStack[-3].GetName(), 'shear')
        self.assertEqual(mayaStack[-2].GetName(), 'scale')
        self.assertEqual(mayaStack[-1].GetName(), 'scalePivot')
        
        def getStackItem(i):
            return mayaStack[i]
        
        self.assertRaises(IndexError, getStackItem, 11)
        self.assertRaises(IndexError, getStackItem, 12)
        self.assertRaises(IndexError, getStackItem, 300)
        self.assertRaises(IndexError, getStackItem, -12)
        self.assertRaises(IndexError, getStackItem, -13)
        self.assertRaises(IndexError, getStackItem, -1000)

    #   UsdMayaXformStack::FindOpIndex
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    def testFindOpIndex(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(mayaStack.FindOpIndex('translate'), 0)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivotTranslate'), 1)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivot'), 2)
        self.assertEqual(mayaStack.FindOpIndex('rotate'), 3)
        self.assertEqual(mayaStack.FindOpIndex('rotateAxis'), 4)
        #assertEqual(mayaStack.FindOpIndex('rotatePivot'), 5)
        self.assertEqual(mayaStack.FindOpIndex('scalePivotTranslate'), 6)
        self.assertEqual(mayaStack.FindOpIndex('scalePivot'), 7)
        self.assertEqual(mayaStack.FindOpIndex('shear'), 8)
        self.assertEqual(mayaStack.FindOpIndex('scale'), 9)
        #assertEqual(mayaStack.FindOpIndex('scalePivot'), 10)

        self.assertEqual(mayaStack.FindOpIndex('translate', isInvertedTwin=False), 0)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivotTranslate', isInvertedTwin=False), 1)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivot', isInvertedTwin=False), 2)
        self.assertEqual(mayaStack.FindOpIndex('rotate', isInvertedTwin=False), 3)
        self.assertEqual(mayaStack.FindOpIndex('rotateAxis', isInvertedTwin=False), 4)
        #assertEqual(mayaStack.FindOpIndex('rotatePivot', isInvertedTwin=False), 5)
        self.assertEqual(mayaStack.FindOpIndex('scalePivotTranslate', isInvertedTwin=False), 6)
        self.assertEqual(mayaStack.FindOpIndex('scalePivot', isInvertedTwin=False), 7)
        self.assertEqual(mayaStack.FindOpIndex('shear', isInvertedTwin=False), 8)
        self.assertEqual(mayaStack.FindOpIndex('scale', isInvertedTwin=False), 9)
        #assertEqual(mayaStack.FindOpIndex('scalePivot', isInvertedTwin=False), 10)

        self.assertEqual(mayaStack.FindOpIndex('translate', False), 0)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivotTranslate', False), 1)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivot', False), 2)
        self.assertEqual(mayaStack.FindOpIndex('rotate', False), 3)
        self.assertEqual(mayaStack.FindOpIndex('rotateAxis', False), 4)
        #assertEqual(mayaStack.FindOpIndex('rotatePivot', False), 5)
        self.assertEqual(mayaStack.FindOpIndex('scalePivotTranslate', False), 6)
        self.assertEqual(mayaStack.FindOpIndex('scalePivot', False), 7)
        self.assertEqual(mayaStack.FindOpIndex('shear', False), 8)
        self.assertEqual(mayaStack.FindOpIndex('scale', False), 9)
        #assertEqual(mayaStack.FindOpIndex('scalePivot', False), 10)

        self.assertIs(mayaStack.FindOpIndex('translate', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOpIndex('rotatePivotTranslate', isInvertedTwin=True), None)
        #self.assertIs(mayaStack.FindOpIndex('rotatePivot', isInvertedTwin=True), 2)
        self.assertIs(mayaStack.FindOpIndex('rotate', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOpIndex('rotateAxis', isInvertedTwin=True), None)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivot', isInvertedTwin=True), 5)
        self.assertIs(mayaStack.FindOpIndex('scalePivotTranslate', isInvertedTwin=True), None)
        #self.assertIs(mayaStack.FindOpIndex('scalePivot', isInvertedTwin=True), 7)
        self.assertIs(mayaStack.FindOpIndex('shear', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOpIndex('scale', isInvertedTwin=True), None)
        self.assertEqual(mayaStack.FindOpIndex('scalePivot', isInvertedTwin=True), 10)

        self.assertIs(mayaStack.FindOpIndex('translate', True), None)
        self.assertIs(mayaStack.FindOpIndex('rotatePivotTranslate', True), None)
        #self.assertIs(mayaStack.FindOpIndex('rotatePivot', True), 2)
        self.assertIs(mayaStack.FindOpIndex('rotate', True), None)
        self.assertIs(mayaStack.FindOpIndex('rotateAxis', True), None)
        self.assertEqual(mayaStack.FindOpIndex('rotatePivot', True), 5)
        self.assertIs(mayaStack.FindOpIndex('scalePivotTranslate', True), None)
        #self.assertIs(mayaStack.FindOpIndex('scalePivot', True), 7)
        self.assertIs(mayaStack.FindOpIndex('shear', True), None)
        self.assertIs(mayaStack.FindOpIndex('scale', True), None)
        self.assertEqual(mayaStack.FindOpIndex('scalePivot', True), 10)
        
        self.assertIs(mayaStack.FindOpIndex('pivot'), None)
        self.assertIs(mayaStack.FindOpIndex('pivot', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOpIndex('pivot', True), None)
        self.assertIs(mayaStack.FindOpIndex('pivot', isInvertedTwin=False), None)
        self.assertIs(mayaStack.FindOpIndex('pivot', False), None)
        
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(commonStack.FindOpIndex('pivot'), 1)
        self.assertEqual(commonStack.FindOpIndex('pivot', isInvertedTwin=True), 4)
        self.assertEqual(commonStack.FindOpIndex('pivot', True), 4)
        self.assertEqual(commonStack.FindOpIndex('pivot', isInvertedTwin=False), 1)
        self.assertEqual(commonStack.FindOpIndex('pivot', False), 1)
        self.assertEqual(commonStack.FindOpIndex('rotate'), 2)
        self.assertIs(commonStack.FindOpIndex('rotate', isInvertedTwin=True), None)
        self.assertIs(commonStack.FindOpIndex('rotate', True), None)
        self.assertEqual(commonStack.FindOpIndex('rotate', isInvertedTwin=False), 2)
        self.assertEqual(commonStack.FindOpIndex('rotate', False), 2)
        self.assertIs(commonStack.FindOpIndex('scalePivot'), None)
        self.assertIs(commonStack.FindOpIndex('scalePivot', isInvertedTwin=True), None)
        self.assertIs(commonStack.FindOpIndex('scalePivot', True), None)
        self.assertIs(commonStack.FindOpIndex('scalePivot', isInvertedTwin=False), None)
        self.assertIs(commonStack.FindOpIndex('scalePivot', False), None)
        
    #   UsdMayaXformStack::FindOpIndex
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformOpClassification::GetName
    #   UsdMayaXformOpClassification::IsInvertedTwin
    def testFindOp(self):
        def getNameInverted(op):
            return (op.GetName(), op.IsInvertedTwin())
        
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(getNameInverted(mayaStack.FindOp('translate')),
                    ('translate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivotTranslate')),
                    ('rotatePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot')),
                    ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotate')),
                    ('rotate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotateAxis')),
                    ('rotateAxis', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot')),
        #            ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivotTranslate')),
                    ('scalePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivot')),
                    ('scalePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('shear')),
                    ('shear', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scale')),
                    ('scale', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('scalePivot')),
        #            ('scalePivot', False))

        self.assertEqual(getNameInverted(mayaStack.FindOp('translate', isInvertedTwin=False)),
                    ('translate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivotTranslate', isInvertedTwin=False)),
                    ('rotatePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', isInvertedTwin=False)),
                    ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotate', isInvertedTwin=False)),
                    ('rotate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotateAxis', isInvertedTwin=False)),
                    ('rotateAxis', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', isInvertedTwin=False)),
        #            ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivotTranslate', isInvertedTwin=False)),
                    ('scalePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', isInvertedTwin=False)),
                    ('scalePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('shear', isInvertedTwin=False)),
                    ('shear', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scale', isInvertedTwin=False)),
                    ('scale', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', isInvertedTwin=False)),
        #            ('scalePivot', False))

        self.assertEqual(getNameInverted(mayaStack.FindOp('translate', False)),
                    ('translate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivotTranslate', False)),
                    ('rotatePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', False)),
                    ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotate', False)),
                    ('rotate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotateAxis', False)),
                    ('rotateAxis', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', False)),
        #            ('rotatePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivotTranslate', False)),
                    ('scalePivotTranslate', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', False)),
                    ('scalePivot', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('shear', False)),
                    ('shear', False))
        self.assertEqual(getNameInverted(mayaStack.FindOp('scale', False)),
                    ('scale', False))
        #assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', False)),
        #            ('scalePivot', False))

        self.assertIs(mayaStack.FindOp('translate', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('rotatePivotTranslate', isInvertedTwin=True), None)
        #self.assertIs(mayaStack.FindOp('rotatePivot', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('rotate', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('rotateAxis', isInvertedTwin=True), None)
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', isInvertedTwin=True)),
                    ('rotatePivot', True))
        self.assertIs(mayaStack.FindOp('scalePivotTranslate', isInvertedTwin=True), None)
        #self.assertIs(mayaStack.FindOp('scalePivot', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('shear', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('scale', isInvertedTwin=True), None)
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', isInvertedTwin=True)),
                    ('scalePivot', True))

        self.assertIs(mayaStack.FindOp('translate', True), None)
        self.assertIs(mayaStack.FindOp('rotatePivotTranslate', True), None)
        #self.assertIs(mayaStack.FindOp('rotatePivot', True), None)
        self.assertIs(mayaStack.FindOp('rotate', True), None)
        self.assertIs(mayaStack.FindOp('rotateAxis', True), None)
        self.assertEqual(getNameInverted(mayaStack.FindOp('rotatePivot', True)),
                    ('rotatePivot', True))
        self.assertIs(mayaStack.FindOp('scalePivotTranslate', True), None)
        #self.assertIs(mayaStack.FindOp('scalePivot', True), None)
        self.assertIs(mayaStack.FindOp('shear', True), None)
        self.assertIs(mayaStack.FindOp('scale', True), None)
        self.assertEqual(getNameInverted(mayaStack.FindOp('scalePivot', True)),
                    ('scalePivot', True))
        
        self.assertIs(mayaStack.FindOp('pivot'), None)
        self.assertIs(mayaStack.FindOp('pivot', isInvertedTwin=True), None)
        self.assertIs(mayaStack.FindOp('pivot', True), None)
        self.assertIs(mayaStack.FindOp('pivot', isInvertedTwin=False), None)
        self.assertIs(mayaStack.FindOp('pivot', False), None)
        
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(getNameInverted(commonStack.FindOp('pivot')),
                    ('pivot', False))
        self.assertEqual(getNameInverted(commonStack.FindOp('pivot', isInvertedTwin=True)),
                    ('pivot', True))
        self.assertEqual(getNameInverted(commonStack.FindOp('pivot', True)),
                    ('pivot', True))
        self.assertEqual(getNameInverted(commonStack.FindOp('pivot', isInvertedTwin=False)),
                    ('pivot', False))
        self.assertEqual(getNameInverted(commonStack.FindOp('pivot', False)),
                    ('pivot', False))
        self.assertEqual(getNameInverted(commonStack.FindOp('rotate')),
                    ('rotate', False))
        self.assertIs(commonStack.FindOp('rotate', isInvertedTwin=True), None)
        self.assertIs(commonStack.FindOp('rotate', True), None)
        self.assertEqual(getNameInverted(commonStack.FindOp('rotate', isInvertedTwin=False)),
                    ('rotate', False))
        self.assertEqual(getNameInverted(commonStack.FindOp('rotate', False)),
                    ('rotate', False))
        self.assertIs(commonStack.FindOp('scalePivot'), None)
        self.assertIs(commonStack.FindOp('scalePivot', isInvertedTwin=True), None)
        self.assertIs(commonStack.FindOp('scalePivot', True), None)
        self.assertIs(commonStack.FindOp('scalePivot', isInvertedTwin=False), None)
        self.assertIs(commonStack.FindOp('scalePivot', False), None)


    #   UsdMayaXformStack::FindOpIndexPair
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    def testFindOpIndexPair(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        self.assertEqual(mayaStack.FindOpIndexPair('translate'), (0, None))
        self.assertEqual(mayaStack.FindOpIndexPair('rotatePivotTranslate'), (1, None))
        self.assertEqual(mayaStack.FindOpIndexPair('rotatePivot'), (2, 5))
        self.assertEqual(mayaStack.FindOpIndexPair('rotate'), (3, None))
        self.assertEqual(mayaStack.FindOpIndexPair('rotateAxis'), (4, None))
        self.assertEqual(mayaStack.FindOpIndexPair('scalePivotTranslate'), (6, None))
        self.assertEqual(mayaStack.FindOpIndexPair('scalePivot'), (7, 10))
        self.assertEqual(mayaStack.FindOpIndexPair('shear'), (8, None))
        self.assertEqual(mayaStack.FindOpIndexPair('scale'), (9, None))

        self.assertEqual(mayaStack.FindOpIndexPair('pivot'), (None, None))
        
        commonStack = UsdMaya.XformStack.CommonStack()
        self.assertEqual(commonStack.FindOpIndexPair('pivot'), (1, 4))
        self.assertEqual(commonStack.FindOpIndexPair('rotate'), (2, None))
        self.assertEqual(commonStack.FindOpIndexPair('scalePivot'), (None, None))

    #   UsdMayaXformStack::FindOpPair
    #   UsdMayaXformStack::MayaStack
    #   UsdMayaXformStack::CommonStack
    #   UsdMayaXformOpClassification::GetName
    #   UsdMayaXformOpClassification::IsInvertedTwin
    def testFindOpPair(self):
        def assertOp(op, expected):
            if expected is None:
                self.assertIs(op, None)
            else:
                self.assertEqual(op.GetName(), expected[0])
                self.assertEqual(op.IsInvertedTwin(), expected[1])
        
        def assertFindOpPair(stack, name, expected0, expected1):
            result = stack.FindOpPair(name)
            self.assertEqual(len(result), 2)
            assertOp(result[0], expected0)
            assertOp(result[1], expected1)
        
        mayaStack = UsdMaya.XformStack.MayaStack()
        assertFindOpPair(mayaStack, 'translate',
                         ('translate', False), None)
        assertFindOpPair(mayaStack, 'rotatePivotTranslate',
                         ('rotatePivotTranslate', False), None)
        assertFindOpPair(mayaStack, 'rotatePivot',
                         ('rotatePivot', False), ('rotatePivot', True))
        assertFindOpPair(mayaStack, 'rotate',
                         ('rotate', False), None)
        assertFindOpPair(mayaStack, 'rotateAxis',
                         ('rotateAxis', False), None)
        assertFindOpPair(mayaStack, 'scalePivotTranslate',
                         ('scalePivotTranslate', False), None)
        assertFindOpPair(mayaStack, 'scalePivot',
                         ('scalePivot', False), ('scalePivot', True))
        assertFindOpPair(mayaStack, 'shear',
                         ('shear', False), None)
        assertFindOpPair(mayaStack, 'scale',
                         ('scale', False), None)

        assertFindOpPair(mayaStack, 'pivot', None, None)
        
        commonStack = UsdMaya.XformStack.CommonStack()
        assertFindOpPair(commonStack, 'pivot',
                         ('pivot', False), ('pivot', True))
        assertFindOpPair(commonStack, 'rotate',
                         ('rotate', False), None)
        assertFindOpPair(commonStack, 'scalePivot', None, None)
        
    def makeMayaStackAttrs(self):
        from pxr import UsdGeom
        
        self.stack = UsdMaya.XformStack.MayaStack()
        self.stage = Usd.Stage.CreateInMemory()
        self.prim = self.stage.DefinePrim('/myPrim', 'Xform')
        self.xform = UsdGeom.Xform(self.prim)
        self.ops = OrderedDict()
        
        self.ops['translate'] = self.xform.AddTranslateOp(opSuffix='translate')
        self.ops['rotatePivotTranslate'] = self.xform.AddTranslateOp(opSuffix='rotatePivotTranslate')
        self.ops['rotatePivot'] = self.xform.AddTranslateOp(opSuffix='rotatePivot')
        self.ops['rotate'] = self.xform.AddRotateXYZOp(opSuffix='rotate')
        self.ops['rotateAxis'] = self.xform.AddRotateXYZOp(opSuffix='rotateAxis')
        self.ops['rotatePivotINV'] = self.xform.AddTranslateOp(opSuffix='rotatePivot', isInverseOp=True )
        self.ops['scalePivotTranslate'] = self.xform.AddTranslateOp(opSuffix='scalePivotTranslate')
        self.ops['scalePivot'] = self.xform.AddTranslateOp(opSuffix='scalePivot')
        self.ops['shear'] = self.xform.AddTransformOp(opSuffix='shear')
        # don't set the opSuffix for scale, just to test an op named "xformOp:scale"
        # instead of "xformOp:scale:scale" (the second form has already been tested with
        # "xformOp:translate:translate")
        self.ops['scale'] = self.xform.AddScaleOp()
        self.ops['scalePivotINV'] = self.xform.AddTranslateOp(opSuffix='scalePivot', isInverseOp=True)
        
    def makeCommonStackAttrs(self):
        from pxr import UsdGeom
        
        self.stack = UsdMaya.XformStack.CommonStack()
        self.stage = Usd.Stage.CreateInMemory()
        self.prim = self.stage.DefinePrim('/myPrim', 'Xform')
        self.xform = UsdGeom.Xform(self.prim)
        self.ops = OrderedDict()

        self.ops['translate'] = self.xform.AddTranslateOp(opSuffix='translate')
        self.ops['pivot'] = self.xform.AddTranslateOp(opSuffix='pivot')
        self.ops['rotate'] = self.xform.AddRotateXYZOp(opSuffix='rotate')
        # don't set the opSuffix for scale, just to test an op named "xformOp:scale"
        # instead of "xformOp:scale:scale" (the second form has already been tested with
        # "xformOp:translate:translate")
        self.ops['scale'] = self.xform.AddScaleOp()
        self.ops['pivotINV'] = self.xform.AddTranslateOp(opSuffix='pivot', isInverseOp=True)
        
    def makeMatrixStackAttrs(self):
        from pxr import UsdGeom
        
        self.stack = UsdMaya.XformStack.MatrixStack()
        self.stage = Usd.Stage.CreateInMemory()
        self.prim = self.stage.DefinePrim('/myPrim', 'Xform')
        self.xform = UsdGeom.Xform(self.prim)
        self.ops = {}

        self.ops['transform'] = self.xform.AddTransformOp(opSuffix='transform')
        
    def makeXformOpsAndExpectedClassifications(self, stack, opNames, expectEmpty=False):
        orderedOps = []
        expected = []
        for opName in opNames:
            orderedOps.append(self.ops[opName])
            if opName.endswith('INV'):
                opName = opName[:-3]
                inverted = True
            else:
                inverted = False
            if not expectEmpty:
                foundOp = stack.FindOp(opName, isInvertedTwin=inverted)
                self.assertIsNotNone(foundOp, "could not find {} op in stack".format(opName))
                expected.append(foundOp)
        return orderedOps, expected
        
    def doSubstackTest(self, stack, opNames, expectEmpty=False, desc=None):
        orderedOps, expected = self.makeXformOpsAndExpectedClassifications(
            stack, opNames, expectEmpty=expectEmpty)
        result = stack.MatchingSubstack(orderedOps)
        self.assertEqual(result, expected, desc)
        
    def testMatchingSubstack_maya_full(self):
        self.makeMayaStackAttrs()
        self.doSubstackTest(self.stack, self.ops.keys())
        
    def testMatchingSubstack_common_full(self):
        self.makeCommonStackAttrs()
        self.doSubstackTest(self.stack, self.ops.keys())
        
    def testMatchingSubstack_matrix_full(self):
        self.makeMatrixStackAttrs()
        self.doSubstackTest(self.stack, self.ops.keys())

    def testMatchingSubstack_maya_empty(self):
        self.makeMayaStackAttrs()
        self.doSubstackTest(self.stack, [], expectEmpty=True)

    def testMatchingSubstack_common_empty(self):
        self.makeCommonStackAttrs()
        self.doSubstackTest(self.stack, [], expectEmpty=True)
        
    def testMatchingSubstack_matrix_empty(self):
        self.makeMatrixStackAttrs()
        self.doSubstackTest(self.stack, [], expectEmpty=True)
        
    def doMissing1SubstackTests(self, stack, allOpNames):
        self.longMessage = True
        pairedIndices = set()
        for invertPair in stack.GetInversionTwins():
            pairedIndices.update(invertPair)
        for i in xrange(len(allOpNames)):
            missingOp = allOpNames[i]
            opsMinus1 = allOpNames[:i] + allOpNames[i + 1:]
            # if the one we're taking out is a member of the inversion twins,
            # then we should get back a non-match
            expectEmpty = i in pairedIndices
            self.doSubstackTest(stack, opsMinus1,
                                expectEmpty=expectEmpty,
                                desc="without {}".format(missingOp))

    def testMatchingSubstack_maya_missing1(self):
        self.makeMayaStackAttrs()
        self.doMissing1SubstackTests(self.stack, self.ops.keys())

    def testMatchingSubstack_common_missing1(self):
        self.makeCommonStackAttrs()
        self.doMissing1SubstackTests(self.stack, self.ops.keys())

    def doOnly1SubstackTests(self, stack, allOpNames):
        self.longMessage = True
        pairedIndices = set()
        for invertPair in stack.GetInversionTwins():
            pairedIndices.update(invertPair)
        for i in xrange(len(allOpNames)):
            onlyOp = allOpNames[i]
            # if the one we're taking is a member of the inversion twins,
            # then we should get back a non-match
            expectEmpty = i in pairedIndices
            self.doSubstackTest(stack, [onlyOp],
                                expectEmpty=expectEmpty,
                                desc="only op: {}".format(onlyOp))

    def testMatchingSubstack_maya_only1(self):
        self.makeMayaStackAttrs()
        self.doOnly1SubstackTests(self.stack, self.ops.keys())

    def testMatchingSubstack_common_only1(self):
        self.makeCommonStackAttrs()
        self.doOnly1SubstackTests(self.stack, self.ops.keys())

    def doTwinSubstackTests(self, stack, allOpNames):
        twins = stack.GetInversionTwins()
        # iterate through all combinations of pairs... practically, for
        # maya, this means we try:
        #   [firstPair]
        #   [secondPair]
        #   [firstPair, secondPair]
        for numPairs in xrange(1, len(twins) + 1):
            for pairSet in itertools.combinations(twins, numPairs):
                usedIndices = set()
                for pair in pairSet:
                    usedIndices.update(pair)
                usedIndices = sorted(usedIndices)
                opNames = [allOpNames[i] for i in usedIndices]
                self.doSubstackTest(stack, opNames,
                                    desc="paired ops: {}".format(opNames))

    def testMatchingSubstack_maya_twins(self):
        self.makeMayaStackAttrs()
        self.doTwinSubstackTests(self.stack, self.ops.keys())

    def testMatchingSubstack_common_twins(self):
        self.makeCommonStackAttrs()
        self.doTwinSubstackTests(self.stack, self.ops.keys())
        
    def testMatchingSubstack_maya_half(self):
        self.makeMayaStackAttrs()
        self.doSubstackTest(self.stack, [
            'rotatePivotTranslate',
            'rotatePivot',
            'rotate',
            'rotatePivotINV',
            'shear',
            'scale',
        ])

    def testMatchingSubstack_common_half(self):
        self.makeCommonStackAttrs()
        self.doSubstackTest(self.stack, [
            'translate',
            'rotate',
            'scale',
        ])
        
    def testMatchingSubstack_wrongOrder(self):
        self.makeMayaStackAttrs()
        self.doSubstackTest(
            self.stack, ['translate', 'rotate'])
        self.doSubstackTest(
            self.stack, ['rotate', 'translate'], expectEmpty=True)
        
        self.doSubstackTest(self.stack, [
            'rotatePivotTranslate',
            'rotatePivot',
            'rotate',
            'rotatePivotINV',
            'scale',
            'shear',
        ], expectEmpty=True)
        
    def testMatchingSubstack_nameDoesntMatter(self):
        self.makeMayaStackAttrs()
        # For the matrix stack, name doesn't matter, so ANY
        # opStack consisting of just a single Transform should match...
        # ...so, ie, just the "shear" transform from the maya stack!
        orderedOps = [self.ops['shear']]
        
        matrixStack = UsdMaya.XformStack.MatrixStack()
        expectedOps = [matrixStack[0]]
        
        # check names doesn't match
        self.assertNotIn(orderedOps[0].GetName(), expectedOps[0].CompatibleAttrNames())
        result = matrixStack.MatchingSubstack(orderedOps)
        
        # ...but we should get back a result anyway!
        self.assertEqual(result, expectedOps)
        
    def testMatchingSubstack_rotOrder(self):
        from maya.OpenMaya import MEulerRotation
        
        self.makeMayaStackAttrs()
        self.ops['rotateX'] = self.xform.AddRotateXOp(opSuffix='rotate')
        self.ops['rotateY'] = self.xform.AddRotateYOp(opSuffix='rotate')
        self.ops['rotateZ'] = self.xform.AddRotateZOp(opSuffix='rotate')
        self.ops['rotateXYZ'] = self.ops['rotate']
        self.ops['rotateYZX'] = self.xform.AddRotateYZXOp(opSuffix='rotate')
        self.ops['rotateZXY'] = self.xform.AddRotateZXYOp(opSuffix='rotate')
        self.ops['rotateXZY'] = self.xform.AddRotateXZYOp(opSuffix='rotate')
        self.ops['rotateYXZ'] = self.xform.AddRotateYXZOp(opSuffix='rotate')
        self.ops['rotateZYX'] = self.xform.AddRotateZYXOp(opSuffix='rotate')
        
        allRotates = [
            'rotateX',
            'rotateY',
            'rotateZ',
            'rotateXYZ',
            'rotateYZX',
            'rotateZXY',
            'rotateXZY',
            'rotateYXZ',
            'rotateZYX',
        ]
        
        expectedList = [self.stack.FindOp('translate'), self.stack.FindOp('rotate')]
        for rotateOpName in allRotates:
            orderedOps = [self.ops['translate'], self.ops[rotateOpName]]
            resultList = self.stack.MatchingSubstack(orderedOps)
            self.assertEqual(resultList, expectedList)
            
        # test a failed match
        orderedOps = [self.ops[rotateOpName], self.ops['translate']]
        resultList = self.stack.MatchingSubstack(orderedOps)
        self.assertEqual(resultList, [])
        
    def testMatchingSubstack_rotOrder_rotAxis(self):
        from maya.OpenMaya import MEulerRotation
        
        self.makeMayaStackAttrs()
        self.ops['rotateAxisX'] = self.xform.AddRotateXOp(opSuffix='rotateAxis')
        self.ops['rotateAxisY'] = self.xform.AddRotateYOp(opSuffix='rotateAxis')
        self.ops['rotateAxisZ'] = self.xform.AddRotateZOp(opSuffix='rotateAxis')
        self.ops['rotateAxisXYZ'] = self.ops['rotateAxis']
        self.ops['rotateAxisYZX'] = self.xform.AddRotateYZXOp(opSuffix='rotateAxis')
        self.ops['rotateAxisZXY'] = self.xform.AddRotateZXYOp(opSuffix='rotateAxis')
        self.ops['rotateAxisXZY'] = self.xform.AddRotateXZYOp(opSuffix='rotateAxis')
        self.ops['rotateAxisYXZ'] = self.xform.AddRotateYXZOp(opSuffix='rotateAxis')
        self.ops['rotateAxisZYX'] = self.xform.AddRotateZYXOp(opSuffix='rotateAxis')
        
        allRotates = {
            'rotateAxisX',
            'rotateAxisY',
            'rotateAxisZ',
            'rotateAxisXYZ',
            'rotateAxisYZX',
            'rotateAxisZXY',
            'rotateAxisXZY',
            'rotateAxisYXZ',
            'rotateAxisZYX',
        }
        
        expectedList = [self.stack.FindOp('translate'), self.stack.FindOp('rotateAxis')]
        for rotateOpName in allRotates:
            orderedOps = [self.ops['translate'], self.ops[rotateOpName]]
            resultList = self.stack.MatchingSubstack(orderedOps)
            self.assertEqual(resultList, expectedList)
            
        # test a failed match
        orderedOps = [self.ops[rotateOpName], self.ops['translate']]
        resultList = self.stack.MatchingSubstack(orderedOps)
        self.assertEqual(resultList, [])
        
    def doFirstMatchingTest(self, stacks, opNames, matchingStack, expectEmpty=False):
        orderedOps, expected = self.makeXformOpsAndExpectedClassifications(
            matchingStack, opNames, expectEmpty=expectEmpty)
        result = UsdMaya.XformStack.FirstMatchingSubstack(stacks, orderedOps)
        self.assertEqual(result, expected)
        
    def testFirstMatchingSubstack(self):
        mayaStack = UsdMaya.XformStack.MayaStack()
        commonStack = UsdMaya.XformStack.CommonStack()
        matrixStack = UsdMaya.XformStack.MatrixStack()
        
        # Should match maya only:
        self.makeMayaStackAttrs()
        mayaOnly = ['translate', 'rotate', 'shear']
        self.doFirstMatchingTest(
            [mayaStack, commonStack, matrixStack],
            mayaOnly, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack, mayaStack, commonStack],
            mayaOnly, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack, commonStack, mayaStack],
            mayaOnly, mayaStack)
        self.doFirstMatchingTest(
            [mayaStack],
            mayaOnly, mayaStack)
        self.doFirstMatchingTest(
            [commonStack],
            mayaOnly, mayaStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack],
            mayaOnly, mayaStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack, mayaStack],
            mayaOnly, mayaStack)
        self.doFirstMatchingTest(
            [commonStack, matrixStack],
            mayaOnly, mayaStack, expectEmpty=True)
        
        # Should match common only:
        self.makeCommonStackAttrs()
        commonOnly = ['pivot', 'pivotINV']
        self.doFirstMatchingTest(
            [commonStack, mayaStack, matrixStack],
            commonOnly, commonStack)
        self.doFirstMatchingTest(
            [matrixStack, commonStack, mayaStack],
            commonOnly, commonStack)
        self.doFirstMatchingTest(
            [matrixStack, mayaStack, commonStack],
            commonOnly, commonStack)
        self.doFirstMatchingTest(
            [mayaStack],
            commonOnly, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack],
            commonOnly, commonStack)
        self.doFirstMatchingTest(
            [matrixStack],
            commonOnly, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack, mayaStack],
            commonOnly, commonStack)
        self.doFirstMatchingTest(
            [mayaStack, matrixStack],
            commonOnly, commonStack, expectEmpty=True)

        # Should match matrix only:
        self.makeMatrixStackAttrs()
        matrixOnly = ['transform']
        self.doFirstMatchingTest(
            [commonStack, mayaStack, matrixStack],
            matrixOnly, matrixStack)
        self.doFirstMatchingTest(
            [commonStack, matrixStack, mayaStack],
            matrixOnly, matrixStack)
        self.doFirstMatchingTest(
            [matrixStack, mayaStack, commonStack],
            matrixOnly, matrixStack)
        self.doFirstMatchingTest(
            [mayaStack],
            matrixOnly, matrixStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack],
            matrixOnly, matrixStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack],
            matrixOnly, matrixStack)
        self.doFirstMatchingTest(
            [commonStack, mayaStack],
            matrixOnly, matrixStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [mayaStack, matrixStack],
            matrixOnly, matrixStack)

        # Should match either maya or common:
        mayaOrCommon = ['translate', 'rotate', 'scale']
        self.makeCommonStackAttrs()
        self.doFirstMatchingTest(
            [commonStack, mayaStack, matrixStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack, commonStack, mayaStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack, mayaStack, commonStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [mayaStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [commonStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack],
            mayaOrCommon, mayaStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack, mayaStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [mayaStack, matrixStack],
            mayaOrCommon, mayaStack)
        self.doFirstMatchingTest(
            [matrixStack, commonStack],
            mayaOrCommon, mayaStack)

        # Should match nothing:
        noMatch = ['translate', 'pivot', 'rotate']
        self.makeCommonStackAttrs()
        self.doFirstMatchingTest(
            [commonStack, mayaStack, matrixStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack, commonStack, mayaStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack, mayaStack, commonStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [mayaStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [commonStack, mayaStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [mayaStack, matrixStack],
            noMatch, commonStack, expectEmpty=True)
        self.doFirstMatchingTest(
            [matrixStack, commonStack],
            noMatch, commonStack, expectEmpty=True)

    def testFirstMatchingSubstack_rotOrder(self):
        from maya.OpenMaya import MEulerRotation
        
        self.longMessage = True
        
        mayaStack = UsdMaya.XformStack.MayaStack()
        commonStack = UsdMaya.XformStack.CommonStack()
        matrixStack = UsdMaya.XformStack.MatrixStack()
        
        self.makeMayaStackAttrs()
        self.ops['rotateX'] = self.xform.AddRotateXOp(opSuffix='rotate')
        self.ops['rotateY'] = self.xform.AddRotateYOp(opSuffix='rotate')
        self.ops['rotateZ'] = self.xform.AddRotateZOp(opSuffix='rotate')
        self.ops['rotateXYZ'] = self.ops['rotate']
        self.ops['rotateYZX'] = self.xform.AddRotateYZXOp(opSuffix='rotate')
        self.ops['rotateZXY'] = self.xform.AddRotateZXYOp(opSuffix='rotate')
        self.ops['rotateXZY'] = self.xform.AddRotateXZYOp(opSuffix='rotate')
        self.ops['rotateYXZ'] = self.xform.AddRotateYXZOp(opSuffix='rotate')
        self.ops['rotateZYX'] = self.xform.AddRotateZYXOp(opSuffix='rotate')
        
        allRotates = {
            'rotateX': MEulerRotation.kXYZ,
            'rotateY': MEulerRotation.kXYZ,
            'rotateZ': MEulerRotation.kXYZ,
            'rotateXYZ': MEulerRotation.kXYZ,
            'rotateYZX': MEulerRotation.kYZX,
            'rotateZXY': MEulerRotation.kZXY,
            'rotateXZY': MEulerRotation.kXZY,
            'rotateYXZ': MEulerRotation.kYXZ,
            'rotateZYX': MEulerRotation.kZYX,
        }
        
        expectedList = [mayaStack.FindOp('translate'), mayaStack.FindOp('rotate')]
        for rotateOpName, expectedRotateOrder in allRotates.iteritems():
            orderedOps = [self.ops['translate'], self.ops[rotateOpName]]
            for stackList in (
                    [],
                    [mayaStack],
                    [commonStack],
                    [matrixStack],
                    [mayaStack, commonStack],
                    [commonStack, mayaStack],
                    [matrixStack, mayaStack],
                    [commonStack, matrixStack],
                    [matrixStack, mayaStack, commonStack],
                    [commonStack, matrixStack, mayaStack],
                    [mayaStack, commonStack, matrixStack]):
                def stackName(x):
                    if x == mayaStack:
                        return 'mayaStack'
                    elif x == commonStack:
                        return 'commonStack'
                    elif x == matrixStack:
                        return 'matrixStack'
                    else:
                        raise ValueError(x)
                stackNames = [stackName(x) for x in stackList]
                errMessage = '\nstackNames: ' + str(stackNames)
                
                expectNone = not (mayaStack in stackList or commonStack in stackList)
                resultList = UsdMaya.XformStack.FirstMatchingSubstack(
                    stackList, orderedOps)
                if expectNone:
                    self.assertEqual(resultList, [], str(stackNames))
                else:
                    self.assertEqual(resultList, expectedList, str(stackNames))
                

if __name__ == '__main__':
    unittest.main(verbosity=2)
