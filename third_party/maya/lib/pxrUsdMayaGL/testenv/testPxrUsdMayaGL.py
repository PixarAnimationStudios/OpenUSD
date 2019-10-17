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
import os
import sys
import unittest

from pxr import Usd

from maya import cmds

class testPxrUsdMayaGL(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('pxrUsd')

    def testEmptySceneDraws(self):
        cmds.file(new=True, force=True)
        usdFilePath = os.path.abspath('blank.usda')
        assembly = cmds.assembly(name='blank', type='pxrUsdReferenceAssembly')
        cmds.setAttr("%s.filePath" % assembly, usdFilePath, type='string')
        cmds.assembly(assembly, edit=True, active='Collapsed')

    def testSimpleSceneDrawsAndReloads(self):
        # a memory issue would sometimes cause maya to crash when opening a new
        # scene.  

        for _ in xrange(20):
            cmds.file(new=True, force=True)
            for i in xrange(10):
                usdFilePath = os.path.abspath('plane%d.usda' % (i%2))
                assembly = cmds.assembly(name='plane', type='pxrUsdReferenceAssembly')
                cmds.setAttr("%s.filePath" % assembly, usdFilePath, type='string')
                cmds.assembly(assembly, edit=True, active='Collapsed')
            cmds.refresh()

        cmds.file(new=True, force=True)

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(testPxrUsdMayaGL)
    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
