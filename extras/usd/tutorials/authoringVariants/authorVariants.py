#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

# Assumes that $USD_INSTALL_ROOT/lib/python is in sys.path

from pxr import Usd, UsdGeom

stage = Usd.Stage.Open('HelloWorld.usda')

# Clear local opinion, which alwauys wins over variants.
colorAttr = UsdGeom.Gprim.Get(stage, '/hello/world').GetDisplayColorAttr()
colorAttr.Clear()
print(stage.GetRootLayer().ExportToString())

# Create variant set.
rootPrim = stage.GetPrimAtPath('/hello')
vset = rootPrim.GetVariantSets().AddVariantSet('shadingVariant')
print(stage.GetRootLayer().ExportToString())

# Create variant options.
vset.AddVariant('red')
vset.AddVariant('blue')
vset.AddVariant('green')
print(stage.GetRootLayer().ExportToString())

# Author red color behind red variant.
vset.SetVariantSelection('red')
with vset.GetVariantEditContext():
    colorAttr.Set([(1,0,0)])
print(stage.GetRootLayer().ExportToString())

vset.SetVariantSelection('blue')
with vset.GetVariantEditContext():
    colorAttr.Set([(0,0,1)])
vset.SetVariantSelection('green')
with vset.GetVariantEditContext():
    colorAttr.Set([(0,1,0)])
print(stage.GetRootLayer().ExportToString())

stage.GetRootLayer().Export('HelloWorldWithVariants.usda')

# Print composed results-- note that variants get flattened away.
print(stage.ExportToString(False))
