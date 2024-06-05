#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# This is an example script from the USD tutorial,
# "Simple Shading in UsdShade".
#
# When run, it will generate a series of usda files in the current
# directory that illustrate each of the steps in the tutorial.
#

from pxr import Gf, Kind, Sdf, Usd, UsdGeom, UsdShade

stage = Usd.Stage.CreateNew("simpleShading.usda")
UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)

# We put both geometry and materials under a common "model root prim",
# which makes it safe to reference the model into another scene.
modelRoot = UsdGeom.Xform.Define(stage, "/TexModel")
Usd.ModelAPI(modelRoot).SetKind(Kind.Tokens.component)

# A simple card with same proportions as the texture we will map
billboard = UsdGeom.Mesh.Define(stage, "/TexModel/card")
billboard.CreatePointsAttr([(-430, -145, 0), (430, -145, 0), (430, 145, 0), (-430, 145, 0)])
billboard.CreateFaceVertexCountsAttr([4])
billboard.CreateFaceVertexIndicesAttr([0,1,2,3])
billboard.CreateExtentAttr([(-430, -145, 0), (430, 145, 0)])
texCoords = UsdGeom.PrimvarsAPI(billboard).CreatePrimvar("st", 
                                    Sdf.ValueTypeNames.TexCoord2fArray, 
                                    UsdGeom.Tokens.varying)
texCoords.Set([(0, 0), (2, 0), (2,2), (0, 2)])

# Now make a Material that contains a PBR preview surface, a texture reader,
# and a primvar reader to fetch the texture coordinate from the geometry
material = UsdShade.Material.Define(stage, '/TexModel/boardMat')
stInput = material.CreateInput('frame:stPrimvarName', Sdf.ValueTypeNames.Token)
stInput.Set('st')

# Create surface, and connect the Material's surface output to the surface 
# shader.  Make the surface non-metallic, and somewhat rough, so it doesn't
# glare in usdview's simple camera light setup.
pbrShader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/PBRShader')
pbrShader.CreateIdAttr("UsdPreviewSurface")
pbrShader.CreateInput("roughness", Sdf.ValueTypeNames.Float).Set(0.4)
pbrShader.CreateInput("metallic", Sdf.ValueTypeNames.Float).Set(0.0)

material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), "surface")

# create texture coordinate reader 
stReader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/stReader')
stReader.CreateIdAttr('UsdPrimvarReader_float2')
# Note here we are connecting the shader's input to the material's 
# "public interface" attribute. This allows users to change the primvar name
# on the material itself without drilling inside to examine shader nodes.
stReader.CreateInput('varname',Sdf.ValueTypeNames.String).ConnectToSource(stInput)

# diffuse texture
diffuseTextureSampler = UsdShade.Shader.Define(stage,'/TexModel/boardMat/diffuseTexture')
diffuseTextureSampler.CreateIdAttr('UsdUVTexture')
diffuseTextureSampler.CreateInput('file', Sdf.ValueTypeNames.Asset).Set("USDLogoLrg.png")
diffuseTextureSampler.CreateInput("st", Sdf.ValueTypeNames.Float2).ConnectToSource(stReader.ConnectableAPI(), 'result')
diffuseTextureSampler.CreateInput("wrapS", Sdf.ValueTypeNames.Token).Set("repeat")
diffuseTextureSampler.CreateInput("wrapT", Sdf.ValueTypeNames.Token).Set("repeat")
diffuseTextureSampler.CreateOutput('rgb', Sdf.ValueTypeNames.Float3)
pbrShader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).ConnectToSource(diffuseTextureSampler.ConnectableAPI(), 'rgb')

# Now bind the Material to the card
billboard.GetPrim().ApplyAPI(UsdShade.MaterialBindingAPI)
UsdShade.MaterialBindingAPI(billboard).Bind(material)

stage.Save()
