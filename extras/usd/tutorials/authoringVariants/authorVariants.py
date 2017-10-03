#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

# Assumes that $USD_INSTALL_ROOT/lib/python is in sys.path

from pxr import Usd, UsdGeom

stage = Usd.Stage.Open('HelloWorld.usda')

# Clear local opinion, which alwauys wins over variants.
colorAttr = UsdGeom.Gprim.Get(stage, '/hello/world').GetDisplayColorAttr()
colorAttr.Clear()
print stage.GetRootLayer().ExportToString()

# Create variant set.
rootPrim = stage.GetPrimAtPath('/hello')
vset = rootPrim.GetVariantSets().AddVariantSet('shadingVariant')
print stage.GetRootLayer().ExportToString()

# Create variant options.
vset.AddVariant('red')
vset.AddVariant('blue')
vset.AddVariant('green')
print stage.GetRootLayer().ExportToString()

# Author red color behind red variant.
vset.SetVariantSelection('red')
with vset.GetVariantEditContext():
    colorAttr.Set([(1,0,0)])
print stage.GetRootLayer().ExportToString()

vset.SetVariantSelection('blue')
with vset.GetVariantEditContext():
    colorAttr.Set([(0,0,1)])
vset.SetVariantSelection('green')
with vset.GetVariantEditContext():
    colorAttr.Set([(0,1,0)])
print stage.GetRootLayer().ExportToString()

stage.GetRootLayer().Export('HelloWorldWithVariants.usda')

# Print composed results-- note that variants get flattened away.
print stage.ExportToString(False)
