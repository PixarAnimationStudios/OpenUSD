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
from pxr import UsdUtils
from pxr import Sdf

def Test(usdFile, outFile, fn):
    layer = Sdf.Layer.FindOrOpen(usdFile)
    UsdUtils.ModifyAssetPaths(layer, fn)
    layer.Export(outFile)

def TestBasic():
    # Test basic substitution
    def fn(s):
        return 'SUCCESS_' + s

    Test('layer.usda', 'modified.usda', fn)

TestBasic()

def TestDuplicates():
    # Test behavior when modify callback yields duplicate
    # asset paths.
    def fn(s):
        return "baz.usd"

    Test('layer.usda', 'duplicates.usda', fn)

TestDuplicates()

def TestRemoval():
    # Tests behavior when modify callback returns empty asset
    # paths.
    def fn(s):
        return ''
        
    Test('layer.usda', 'removal.usda', fn)

TestRemoval()

def TestDoesNotRecurseDeps():
    # Tests reference paths are not traversed during asset path modification
    def fn(s):
        return 'MOD_' + s
    
    refLayer = Sdf.Layer.FindOrOpen('ref.usda')
    Test('ref_layer.usda', 'ref_layer_mod.usda', fn)
    refLayer.Export("ref_unmodified.usda")

TestDoesNotRecurseDeps()
