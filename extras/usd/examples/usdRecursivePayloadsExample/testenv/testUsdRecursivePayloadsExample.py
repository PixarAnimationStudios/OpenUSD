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

