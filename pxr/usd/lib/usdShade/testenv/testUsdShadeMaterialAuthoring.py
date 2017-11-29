#!/pxrpythonsubst                                                                   
#                                                                                   
# Copyright 2017 Pixar                                                              
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

from pxr import Sdf, Usd, UsdGeom, UsdShade
import unittest

class TestUsdShadeMaterialAuthoring(unittest.TestCase):
    def test_Basic(self):
        # There are a number of ways we could vary shading between wet and dry...
        # We're choosing the biggest hammer, which is to completely swap out
        # the surface shader (which is how it has been done in our pipeline)
        shadeVariations = ["Wet", "Dry"]

        # For each "base" we will have a Material, and a single Shader as the surface.
        # In reality there's likely be many more shading components/prims feeding
        # the surfaces.
        materialBases = ["Hair", "Skin", "Nails"]

        shadersPath = Sdf.Path("/ShadingDefs/Shaders")
        materialsPath   = Sdf.Path("/ShadingDefs/Materials")

        def MakeSurfacePath(base, variant, prop = None):
            retval = shadersPath.AppendChild(base + variant + "Surface")
            if prop:
                retval = retval.AppendProperty(prop)
            return retval

        def MakeDisplacementPath(base, variant, prop = None):
            retval = shadersPath.AppendChild(base + variant + "Disp")
            if prop:
                retval = retval.AppendProperty(prop)
            return retval

        def MakePatternPath(base, variant, prop = None):
            retval = shadersPath.AppendChild(base + variant + "Pattern")
            if prop:
                retval = retval.AppendProperty(prop)
            return retval

        def MakeMaterialPath(base, prop = None):
            retval = materialsPath.AppendChild(base + "Material")
            if prop:
                retval = retval.AppendProperty(prop)
            return retval

        #def CreateTerminal(material, name, targetPath):
            #terminalRel = material.GetPrim().CreateRelationship("terminal:%s" % name)
            #terminalRel.SetTargets([targetPath,]);
            #return terminalRel

        def SetupShading(stage):
            # First create the shading prims
            UsdGeom.Scope.Define(stage, shadersPath)
            UsdGeom.Scope.Define(stage, materialsPath)
            # .. and as we create each surface, bind the associated material's variant to it
            allMaterials = []
            for material in materialBases:
                materialPrim = UsdShade.Material.Define(stage, MakeMaterialPath(material))
                allMaterials.append(materialPrim.GetPrim())
                for variant in shadeVariations:
                    surface = UsdShade.Shader.Define(
                        stage, MakeSurfacePath(material, variant))
                    colorOut = surface.CreateOutput("out", Sdf.ValueTypeNames.Color3f)

                    disp = UsdShade.Shader.Define(
                        stage, MakeDisplacementPath(material, variant))
                    dispOut = disp.CreateOutput('out', Sdf.ValueTypeNames.Vector3f)

                    pattern = UsdShade.Shader.Define(
                        stage, MakePatternPath(material, variant))
                    patternOut = pattern.CreateOutput("out", 
                        Sdf.ValueTypeNames.FloatArray)

                    with materialPrim.GetEditContextForVariant(variant):
                        surfaceOutput = materialPrim.CreateOutput("surface",
                            colorOut.GetTypeName())
                        surfaceOutput.ConnectToSource(
                            UsdShade.ConnectableAPI(colorOut.GetPrim()),
                            colorOut.GetBaseName(), 
                            UsdShade.AttributeType.Output)

                        displacementOutput = materialPrim.CreateOutput("displacement", 
                            dispOut.GetTypeName())
                        displacementOutput.ConnectToSource(dispOut)

                        patternOutput = materialPrim.CreateOutput("pattern", 
                            patternOut.GetTypeName())
                        patternOutput.ConnectToSource(patternOut)
                        
                        # XXX: If we replace these terminals with UsdShadeOutput's, then
                        ## we can't have these point to prim paths.
                        #surfacePath = MakeSurfacePath(material, variant, 'out')
                        #CreateTerminal(materialPrim, "surface", surfacePath)

                        #dispPath = MakeDisplacementPath(material, variant, 'out')
                        #CreateTerminal(materialPrim, "displacement", dispPath)

                        #CreateTerminal(materialPrim, 'pattern', 
                                       #MakePatternPath(material, variant, 'out'))

            return allMaterials

        def ValidateMaterial(stage):
            hairPrim = stage.GetPrimAtPath('/ModelShading/Materials/HairMaterial')
            hairMaterial = UsdShade.Material(hairPrim)
            self.assertTrue(hairMaterial)
            wetHairSurfPath = Sdf.Path('/ModelShading/Shaders/HairWetSurface.out')
            wetHairDispPath = Sdf.Path('/ModelShading/Shaders/HairWetDisp.out')
            wetHairPatternPath = Sdf.Path('/ModelShading/Shaders/HairWetPattern.out')

            connectedSurface = hairMaterial.GetOutput('surface').GetConnectedSource()
            connectedSurfacePath = connectedSurface[0].GetPath().AppendProperty(
                connectedSurface[1])
            self.assertEqual(connectedSurfacePath, wetHairSurfPath)

            connectedDisplacement = hairMaterial.GetOutput('displacement').GetConnectedSource()
            connectedDisplacementPath = connectedDisplacement[0].GetPath().AppendProperty(
                connectedDisplacement[1])
            self.assertEqual(connectedDisplacementPath, wetHairDispPath)

            connectedPattern = hairMaterial.GetOutput('pattern').GetConnectedSource()
            connectedPatternPath = connectedPattern[0].GetPath().AppendProperty(
                connectedPattern[1])
            self.assertEqual(connectedPatternPath, wetHairPatternPath)

            # change the root-level variantSet, which should in turn change the Material's
            self.assertTrue(rootPrim.GetVariantSets().SetSelection("materialVariant", "Dry"))
            self.assertTrue(hairMaterial)
            dryHairSurfPath = Sdf.Path('/ModelShading/Shaders/HairDrySurface.out')
            dryHairDispPath = Sdf.Path('/ModelShading/Shaders/HairDryDisp.out')
            dryHairPatternPath = Sdf.Path('/ModelShading/Shaders/HairDryPattern.out')

            connectedSurface = hairMaterial.GetOutput('surface').GetConnectedSource()
            connectedSurfacePath = connectedSurface[0].GetPath().AppendProperty(
                connectedSurface[1])
            self.assertEqual(connectedSurfacePath, dryHairSurfPath)

            connectedDisplacement = hairMaterial.GetOutput('displacement').GetConnectedSource()
            connectedDisplacementPath = connectedDisplacement[0].GetPath().AppendProperty(
                connectedDisplacement[1])
            self.assertEqual(connectedDisplacementPath, dryHairDispPath)

            connectedPattern = hairMaterial.GetOutput('pattern').GetConnectedSource()
            connectedPatternPath = connectedPattern[0].GetPath().AppendProperty(
                connectedPattern[1])
            self.assertEqual(connectedPatternPath, dryHairPatternPath)

        fileName = "char_shading.usda"
        stage = Usd.Stage.CreateNew(fileName)

        # Create this prim first, since it's the "entrypoint" to the layer, and
        # we want it to appear at the top
        rootPrim = stage.DefinePrim("/ModelShading")

        # Next, create a tree that will "sit on top of ShadingDefs" to switch the
        # materials in concert
        allMaterials = SetupShading(stage)
        bindingVariantRoot = stage.OverridePrim("/MaterialBindingVariants")
        UsdShade.Material.CreateMasterMaterialVariant(bindingVariantRoot, allMaterials)

        # Finally, this is how we stitch them together into an interface.
        # This is the root prim that a client would target to pull in shading
        refs = rootPrim.GetReferences()
        # XXX We need a better way of specifying self-references
        refs.AddReference("./"+fileName, "/ShadingDefs",
            position=Usd.ListPositionFront)
        refs.AddReference("./"+fileName, "/MaterialBindingVariants",
            position=Usd.ListPositionFront)

        stage.GetRootLayer().Save()

        # Now let's do some validation that it performs as expected
        ValidateMaterial(stage)


        # Now let's make a variation of the above in which we do the master variant
        # on a composed stage in which rootPrim is already referencing the ShadingDefs,
        # and operating on the composed Material prims
        fileName = "char_shading_compact.usda"
        stage = Usd.Stage.CreateNew(fileName)

        # Create this prim first, since it's the "entrypoint" to the layer, and
        # we want it to appear at the top
        rootPrim = stage.DefinePrim("/ModelShading")

        SetupShading(stage)
        # Reference the shading directly
        refs = rootPrim.GetReferences()
        refs.AddReference("./"+fileName, "/ShadingDefs",
            position=Usd.ListPositionFront)

        # Now pick up the newly composed material prims
        allMaterials = [ stage.GetPrimAtPath("/ModelShading/Materials/HairMaterial"),
                     stage.GetPrimAtPath("/ModelShading/Materials/SkinMaterial"),
                     stage.GetPrimAtPath("/ModelShading/Materials/NailsMaterial") ]

        UsdShade.Material.CreateMasterMaterialVariant(rootPrim, allMaterials)

        stage.GetRootLayer().Save()

        # Now let's do some validation that it performs as expected
        ValidateMaterial(stage)

        # TODO: move this into it's own test
        hairPrim = stage.GetPrimAtPath('/ModelShading/Materials/HairMaterial')
        hairMaterial = UsdShade.Material(hairPrim)
        interfaceInput = hairMaterial.CreateInput("myParam", Sdf.ValueTypeNames.Float)
        interfaceInput.SetDocumentation("this is my float")
        interfaceInput.SetDisplayGroup("numbers")

        byName = hairMaterial.GetInput("myParam")
        self.assertTrue(byName and 
            ( byName.GetDocumentation() == "this is my float") and 
            ( byName.GetDisplayGroup() == "numbers"))

        plain = hairPrim.GetAttribute(byName.GetFullName())
        self.assertTrue(plain and ( plain.GetTypeName() == "float"))
        
if __name__ == "__main__":
    unittest.main()
