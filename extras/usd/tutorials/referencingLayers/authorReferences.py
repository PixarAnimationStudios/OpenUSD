#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Assumes that $USD_INSTALL_ROOT/lib/python is in sys.path

from __future__ import print_function

from pxr import Usd, UsdGeom

stage = Usd.Stage.Open('HelloWorld.usda')
# XXX: save the usd file to the local directory where the test
# is being administered so RefExample will have access to it
stage.Export('HelloWorld.usda')
hello = stage.GetPrimAtPath('/hello')
stage.SetDefaultPrim(hello)
UsdGeom.XformCommonAPI(hello).SetTranslate((4, 5, 6))
print(stage.GetRootLayer().ExportToString())
stage.GetRootLayer().Save()

# Create an over on which to create our reference.
refStage = Usd.Stage.CreateNew('RefExample.usda')
refSphere = refStage.OverridePrim('/refSphere')
print(refStage.GetRootLayer().ExportToString())

# Reference in the layer.
refSphere.GetReferences().AddReference('./HelloWorld.usda')
print(refStage.GetRootLayer().ExportToString())
refStage.GetRootLayer().Save()

# Clear out any authored transformation over the reference.
refXform = UsdGeom.Xformable(refSphere)
refXform.SetXformOpOrder([])
print(refStage.GetRootLayer().ExportToString())

# Reference in the layer again, on another over.
refSphere2 = refStage.OverridePrim('/refSphere2')
refSphere2.GetReferences().AddReference('./HelloWorld.usda')
print(refStage.GetRootLayer().ExportToString())
refStage.GetRootLayer().Save()

# Author displayColor on a namespace descendant of the referenced prim.
overSphere = UsdGeom.Sphere.Get(refStage, '/refSphere2/world' )
overSphere.GetDisplayColorAttr().Set( [(1, 0, 0)] )
print(refStage.GetRootLayer().ExportToString())
refStage.GetRootLayer().Save()

# Print the final composed results.
print(refStage.ExportToString())
