#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Sdf, Usd, Plug
import os

# Register the file format plugin which should not have been automatically 
# loaded.
Plug.Registry().RegisterPlugins(os.getcwd())

stage = Usd.Stage.Open('test_root.usda')
assert stage

# The example file format should be loaded and exist by now
dynamicFormat = Sdf.FileFormat.FindByExtension("usdrecursivepayloadsexample")
assert dynamicFormat

# Export the flattened stage for baseline comparison
flattened = stage.Flatten(addSourceFileComment=False)
flattened.Export("flattenedContents.usda")

# Change the payload prim metadata parameters which will change the dynamic
# layer's contents.
p = stage.GetPrimAtPath('/RootMulti')
p.SetMetadata("UsdExample_radius", 25.0)
p.SetMetadata("UsdExample_depth", 3)

# Export the new flattened stage for baseline comparison
newFlattened = stage.Flatten(addSourceFileComment=False)
newFlattened.Export("newFlattenedContents.usda")

# Verify Capabilities
assert Sdf.FileFormat.FormatSupportsReading('.usdrecursivepayloadsexample') == True
assert Sdf.FileFormat.FormatSupportsWriting('.usdrecursivepayloadsexample') == False
assert Sdf.FileFormat.FormatSupportsEditing('.usdrecursivepayloadsexample') == False

