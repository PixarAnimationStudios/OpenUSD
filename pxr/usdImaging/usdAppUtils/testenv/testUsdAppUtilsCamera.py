#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Sdf
from pxr import Tf
from pxr import Usd
from pxr import UsdAppUtils

import os
import unittest


class TestUsdAppUtilsCamera(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        usdFilePath = os.path.abspath('Cameras.usda')
        cls._stage = Usd.Stage.Open(usdFilePath)

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testGetCameraAbsolutePath(self):
        """
        Tests getting cameras using an absolute path.
        """
        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            '/Cameras/MainCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString, '/Cameras/MainCamera')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            '/Cameras/Deeply/Nested/CameraB')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/Cameras/Deeply/Nested/CameraB')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            '/OtherScope/OtherCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/OtherScope/OtherCamera')

    def testGetCameraByName(self):
        """
        Tests getting cameras using just the camera's prim name. The stage
        should be searched for the camera prim matching that name.
        """
        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, 'MainCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString, '/Cameras/MainCamera')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, 'CameraB')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/Cameras/Deeply/Nested/CameraB')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, 'OtherCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/OtherScope/OtherCamera')

    def testGetCameraByMultiElementPath(self):
        """
        Tests getting cameras using a multi-element path. Currently these paths
        are just made absolute using the absolute root path before searching.
        """
        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            'Cameras/MainCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString, '/Cameras/MainCamera')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            'Cameras/Deeply/Nested/CameraB')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/Cameras/Deeply/Nested/CameraB')

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage,
            'OtherScope/OtherCamera')
        self.assertTrue(usdCamera)
        self.assertEqual(usdCamera.GetPath().pathString,
            '/OtherScope/OtherCamera')

    def testGetCameraBadParams(self):
        """
        Tests trying to get cameras that don't exist and using invalid
        parameters.
        """
        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, 'BogusCamera')
        self.assertFalse(usdCamera)

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, Sdf.Path.emptyPath)
        self.assertFalse(usdCamera)

        usdCamera = UsdAppUtils.GetCameraAtPath(self._stage, 'foo.bar')
        self.assertFalse(usdCamera)

        with self.assertRaises(Tf.ErrorException):
            UsdAppUtils.GetCameraAtPath(None, 'MainCamera')


if __name__ == "__main__":
    unittest.main()
