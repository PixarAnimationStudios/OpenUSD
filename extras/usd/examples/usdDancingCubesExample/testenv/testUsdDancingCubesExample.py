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

stage = Usd.Stage.Open('test.usda')
assert stage

# The example file format should be loaded and exist by now
dynamicFormat = Sdf.FileFormat.FindByExtension("usddancingcubesexample")
assert dynamicFormat

# Find the layer that matches the dynamic file format. There should be exactly
# one.
dynamicLayers = list(filter(lambda l: l.GetFileFormat() == dynamicFormat, 
                            stage.GetUsedLayers()))
assert len(dynamicLayers) == 1

# Export the dynamic layer as usda for baseline comparison
dynamicLayers[0].Export("dynamicContents.usda")

# Change the payload prim metadata parameters which will change the dynamic 
# layer's contents.
p = stage.GetPrimAtPath('/Root')
md = p.GetMetadata("Usd_DCE_Params")
p.SetMetadata("Usd_DCE_Params",
              {'geomType': 'Cone', 
               'distance': 4.0, 
               'numFrames': 20, 
               'perSide': 3, 
               'framesPerCycle': 12, 
               'moveScale': 4.5 })

# Find the dynamic layer again. Note that this layer has a different identity
# as the file format arguments have changed.
dynamicLayers = list(filter(lambda l: l.GetFileFormat() == dynamicFormat, 
                            stage.GetUsedLayers()))
assert len(dynamicLayers) == 1

# Export the new dynamic layer as usda for baseline comparison.
dynamicLayers[0].Export("newDynamicContents.usda")

