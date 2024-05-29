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
              { 'distance': 4.0, 
               'numFrames': 20, 
               'perSide': 3, 
               'framesPerCycle': 12, 
               'moveScale': 4.5 })

# Confirm that variants affect dynamic payload arguments
p.GetVariantSets().GetVariantSet("geomTypeVariant").SetVariantSelection("sphere")

# Find the dynamic layer again. Note that this layer has a different identity
# as the file format arguments have changed.
dynamicLayers = list(filter(lambda l: l.GetFileFormat() == dynamicFormat, 
                            stage.GetUsedLayers()))
assert len(dynamicLayers) == 1

# Export the new dynamic layer as usda for baseline comparison.
dynamicLayers[0].Export("newDynamicContents.usda")

# Change all the parametes back to the original values but do it by creating
# attributes for each parameter and setting default values. Default values from
# attributes will take precedence over the parameters in the metadata 
# dictionary.
p = stage.GetPrimAtPath('/Root')
p.CreateAttribute('geomType', Sdf.ValueTypeNames.Token).Set("Cube")
p.CreateAttribute('distance', Sdf.ValueTypeNames.Double).Set(6.0)
p.CreateAttribute('numFrames', Sdf.ValueTypeNames.Int).Set(20)
p.CreateAttribute('perSide', Sdf.ValueTypeNames.Int).Set(4)
p.CreateAttribute('framesPerCycle', Sdf.ValueTypeNames.Int).Set(16)
p.CreateAttribute('moveScale', Sdf.ValueTypeNames.Double).Set(1.5)

# Find the dynamic layer again. Note that this layer has a different identity
# as the file format arguments have changed.
dynamicLayers = list(filter(lambda l: l.GetFileFormat() == dynamicFormat, 
                            stage.GetUsedLayers()))
assert len(dynamicLayers) == 1

# Export the new dynamic layer as usda for baseline comparison.
dynamicLayers[0].Export("dynamicContentsFromAttrs.usda")

# Verify mute and unmute behavior on the dynamic layer itself. A muted layer
# is empty (except the pseudoroot)
premuteLayerString = dynamicLayers[0].ExportToString()
dynamicLayers[0].SetMuted(True)
assert dynamicLayers[0].ExportToString().rstrip() == "#usda 1.0"
dynamicLayers[0].SetMuted(False)
assert dynamicLayers[0].ExportToString() == premuteLayerString

# Verify Capabilities
assert Sdf.FileFormat.FormatSupportsReading('.usddancingcubesexample') == True
assert Sdf.FileFormat.FormatSupportsWriting('.usddancingcubesexample') == False
assert Sdf.FileFormat.FormatSupportsEditing('.usddancingcubesexample') == False

