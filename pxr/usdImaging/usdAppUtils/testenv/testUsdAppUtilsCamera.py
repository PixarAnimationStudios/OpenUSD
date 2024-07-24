#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
