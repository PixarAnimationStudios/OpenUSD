#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from maya import cmds
from maya import standalone

from pxr import Usd

class testUsdExportEulerFilter(unittest.TestCase):
    longMessage = True

    expectedNoFilter = {
        1.0: (-0.21669838, -1.8291378, 174.18279),
        2.0: (0.027377421, -6.3026876, -179.2725),
        3.0: (0.26995704, 0.286542, -172.7325),
    }

    expectedFilter = {
        1.0: (-0.21669838, -1.8291378, 174.18279),
        2.0: (0.027377421, -6.3026876, 180.7275),
        3.0: (0.26995704, 0.286542, 187.2675),
    }
        
    def assertRotateSamples(self, usdFile, expected):
        stage = Usd.Stage.Open(usdFile)
        
        xform = stage.GetPrimAtPath('/pCube1')
        rot = xform.GetProperty('xformOp:rotateXYZ')

        for time, expectedVec in expected.iteritems():
            actualVec = rot.Get(time)
            for i in xrange(3):
                msg = "sample at time {}, index {} unequal".format(time, i)
                self.assertAlmostEqual(actualVec[i], expectedVec[i], 4, msg)

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportEulerFilterTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def testExportEulerFilterCmd(self):
        '''
        Export a constrained xform with and without eulerFiltering
        '''
        usdFile = os.path.abspath('UsdExportEulerFilterTestCmd.usda')

        # Test with no option set (defaults to off)
        cmds.select('pCube1')
        cmds.usdExport(file=usdFile, shadingMode='none',
                       frameRange=(1.0, 3.0), selection=1)
        self.assertRotateSamples(usdFile, self.expectedNoFilter)

        # Test with eulerFilter on
        cmds.select('pCube1')
        cmds.usdExport(file=usdFile, shadingMode='none', eulerFilter=True,
                       frameRange=(1.0, 3.0), selection=1)
        self.assertRotateSamples(usdFile, self.expectedFilter)

        # Test with eulerFilter off
        cmds.select('pCube1')
        cmds.usdExport(file=usdFile, shadingMode='none', eulerFilter=False,
                       frameRange=(1.0, 3.0), selection=1)
        self.assertRotateSamples(usdFile, self.expectedNoFilter)

    def testExportEulerFilterTranslator(self):
        '''
        Export a constrained xform with and without eulerFiltering
        '''
        usdFile = os.path.abspath('UsdExportEulerFilterTestTranslator.usda')

        # Test with no option set (defaults to off)
        options = [
            'shadingMode=none',
            'animation=1',
            'startTime=1',
            'endTime=3',
        ]
        cmds.select('pCube1')
        cmds.file(usdFile, exportSelected=1, options=';'.join(options),
                  type='pxrUsdExport', f=1)
        self.assertRotateSamples(usdFile, self.expectedNoFilter)

        # Test with eulerFilter on
        options.append('eulerFilter=1')
        cmds.select('pCube1')
        cmds.file(usdFile, exportSelected=1, options=';'.join(options),
                  type='pxrUsdExport', f=1)
        self.assertRotateSamples(usdFile, self.expectedFilter)

        # Test with eulerFilter off
        options[-1] = 'eulerFilter=0'
        cmds.select('pCube1')
        cmds.file(usdFile, exportSelected=1, options=';'.join(options),
                  type='pxrUsdExport', f=1)
        self.assertRotateSamples(usdFile, self.expectedNoFilter)

if __name__ == '__main__':
    unittest.main(verbosity=2)
