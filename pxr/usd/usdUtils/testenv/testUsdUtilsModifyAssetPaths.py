#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import UsdUtils
from pxr import Sdf

def Test(usdFile, outFile, fn, keepEmptyPathsInArrays = False):
    layer = Sdf.Layer.FindOrOpen(usdFile)
    UsdUtils.ModifyAssetPaths(layer, fn, keepEmptyPathsInArrays)
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

def TestRemovalPreserveLength():
    # Tests behavior when modify callback returns empty asset
    # paths and preserve array length is enabled.
    def fn(s):
        return ''
        
    Test('layer.usda', 'removal_preserve_length.usda', fn, 
         keepEmptyPathsInArrays=True)

TestRemovalPreserveLength()

def TestDoesNotRecurseDeps():
    # Tests reference paths are not traversed during asset path modification
    def fn(s):
        return 'MOD_' + s
    
    refLayer = Sdf.Layer.FindOrOpen('ref.usda')
    Test('ref_layer.usda', 'ref_layer_mod.usda', fn)
    refLayer.Export("ref_unmodified.usda")

TestDoesNotRecurseDeps()

def TestDoesNotStripMetadata():
    # Ensure that metadata is retained.
    def fn(s):
        return s

    Test('metadata.usda', 'preserve_metadata.usda', fn)

TestDoesNotStripMetadata()
