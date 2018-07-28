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

from pxr import Usd, UsdGeom


class testUsdExportPackage(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(os.path.abspath('PackageTest.ma'), open=True, force=True)

    def testExport(self):
        usdFile = os.path.abspath('MyAwesomePackage.usdz')
        cmds.usdExport(
                file=usdFile,
                mergeTransformAndShape=True,
                shadingMode='none')

        # Lets make sure that the root layer is the first file and that all
        # the references were localized ok.
        zipFile = Usd.ZipFile.Open(usdFile)
        fileNames = zipFile.GetFileNames()
        self.assertEqual(fileNames, [
            "MyAwesomePackage.usd",
            "ReferenceModel.usda",
            "BaseModel.usda",
            "card.png"
        ])

        # Open the usdz file up to verify that everything exported properly.
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage)

        self.assertTrue(stage.GetPrimAtPath("/PackageTest"))
        self.assertTrue(stage.GetPrimAtPath("/PackageTest/MyAssembly"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1/Geom/MyMesh"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel2"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1/Geom/MyMesh"))

        prim = stage.GetPrimAtPath("/PackageTest/Sphere")
        self.assertTrue(prim)
        modelAPI = UsdGeom.ModelAPI(prim)
        self.assertEqual(modelAPI.GetModelCardTextureXNegAttr().Get().path,
                "./card.png")

        # Make sure there's no weird temp files sitting around.
        lsDir = os.listdir(os.path.dirname(usdFile))
        for item in lsDir:
            self.assertNotRegexpMatches(item, "tmp-.*\.usd")


if __name__ == '__main__':
    unittest.main(verbosity=2)
