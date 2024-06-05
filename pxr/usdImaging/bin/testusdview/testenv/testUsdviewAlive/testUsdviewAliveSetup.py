#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Usd, UsdGeom

s = Usd.Stage.CreateInMemory()
UsdGeom.Mesh.Define(s, '/mesh')
s.GetRootLayer().Export('test.usda')
