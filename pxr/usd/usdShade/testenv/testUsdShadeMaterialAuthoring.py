#!/pxrpythonsubst                                                              
#                                                                              
# Copyright 2017 Pixar                                                         
#                                                                              
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdGeom, UsdShade
import unittest

class TestUsdShadeMaterialAuthoring(unittest.TestCase):
    def test_Basic(self):

        shadingDefs = Sdf.Path("/ShadingDefs")

        # There are a number of ways we could vary shading between wet and dry.
        # We're choosing the biggest hammer, which is to completely swap out
        # the surface shader (which is how it has been done in Pixar's pipeline)
        shadeVariations = ["Wet", "Dry"]

        # For each "base" we will have a Material, and a single Shader as the
        # surface.  In reality there's likely be many more shading
        # components/prims feeding the surfaces.
        materialBases = ["Hair", "Skin", "Nails"]

        # We often create Shaders "inline" under the Material that uses them,
        # but here we put all Shaders in a common, external prim that will
        # be referenced into each Material; since we will be creating inline 
        # VariantSets for the materials, this structure ensures the Shader
        # definitions are weaker than any of the Variant opinions.
        shadersPath   = shadingDefs.AppendChild("Shaders")
        materialsPath = shadingDefs.AppendChild("Materials")

        # If `referencedPropName` is provided, construct a path to that
        # prop in the *referenced location of the implied shader.  Otherwise,
        # return a path to the _definition_ for the shader.
        def MakeShadingPath(base, variant, category, referencedPropName = None):
            if referencedPropName:
                return materialsPath.AppendChild(base + "Material")\
                                    .AppendChild(base + variant + category)\
                                    .AppendProperty(referencedPropName)
            else:
                return shadersPath.AppendChild(base + variant + category)

        def MakeSurfacePath(base, variant, referencedPropName = None):
            return MakeShadingPath(base, variant, "Surface", referencedPropName)

        def MakeDisplacementPath(base, variant, referencedPropName = None):
            return MakeShadingPath(base, variant, "Displacement", 
                                   referencedPropName)

        def MakeMaterialPath(base, prop = None):
            retval = materialsPath.AppendChild(base + "Material")
            if prop:
                retval = retval.AppendProperty(prop)
            return retval

        def SetupShading(stage):
            # First create the shading prims
            UsdGeom.Scope.Define(stage, shadersPath)
            UsdGeom.Scope.Define(stage, materialsPath)

            allMaterials = []
            for material in materialBases:
                # Create and remember each Material, and reference in the
                # complete set of shaders onto each Material prim
                materialPrim = UsdShade.Material.Define(stage, 
                        MakeMaterialPath(material))
                materialPrim.GetPrim().\
                    GetReferences().AddInternalReference(shadersPath)
                allMaterials.append(materialPrim.GetPrim())

                for variant in shadeVariations:
                    # .. and as we create each surface, bind the associated
                    # material's variant to it
                    surface = UsdShade.Shader.Define(
                        stage, MakeSurfacePath(material, variant))
                    surfaceOut = surface.CreateOutput("surface", 
                                                      Sdf.ValueTypeNames.Token)

                    disp = UsdShade.Shader.Define(
                        stage, MakeDisplacementPath(material, variant))
                    dispOut = disp.CreateOutput("displacement", 
                                                Sdf.ValueTypeNames.Token)

                    with materialPrim.GetEditContextForVariant(variant):
                        surfaceOutput = materialPrim.CreateSurfaceOutput()
                        surfaceOutput.ConnectToSource(
                            MakeSurfacePath(material, 
                                            variant,
                                            surfaceOut.GetFullName()))

                        displacementOutput = materialPrim.\
                                             CreateDisplacementOutput()
                        displacementOutput.ConnectToSource(
                            MakeDisplacementPath(material, 
                                                 variant,
                                                 dispOut.GetFullName()))

            # Change root prim to an over so it is not traversed by the stage
            stage.GetPrimAtPath(shadingDefs).SetSpecifier(Sdf.SpecifierOver)
            
            return allMaterials

        def ValidateMaterial(stage):
            hairPath = Sdf.Path('/ModelShading/Materials/HairMaterial')
            hairPrim = stage.GetPrimAtPath(hairPath)
            hairMaterial = UsdShade.Material(hairPrim)
            self.assertTrue(hairMaterial)

            # Validate wet surface terminal connection
            wetHairSurfPath = hairPath.AppendChild('HairWetSurface')\
                                      .AppendProperty("outputs:surface")
            surfaceTerminal = hairMaterial.GetSurfaceOutput()
            self.assertTrue(surfaceTerminal.HasConnectedSource())
            connectedSurfaceInfo = surfaceTerminal.GetConnectedSources()[0][0]
            targetSurfacePath = UsdShade.Utils.GetConnectedSourcePath(connectedSurfaceInfo)
            self.assertEqual(targetSurfacePath, wetHairSurfPath)


            # Validate wet displacement terminal connection
            wetHairDispPath = hairPath.AppendChild('HairWetDisplacement')\
                                      .AppendProperty("outputs:displacement")
            dispTerminal = hairMaterial.GetDisplacementOutput()
            self.assertTrue(dispTerminal.HasConnectedSource())
            connectedDispInfo = dispTerminal.GetConnectedSources()[0][0]
            
            targetDispPath = UsdShade.Utils.GetConnectedSourcePath(connectedDispInfo)
            self.assertEqual(targetDispPath, wetHairDispPath)


            # change the root-level variantSet, which should in turn change
            # the Material's
            self.assertTrue(rootPrim\
                            .GetVariantSets()\
                            .SetSelection("materialVariant", "Dry"))
            self.assertTrue(hairMaterial)

            # Validate dry surface terminal connection
            dryHairSurfPath = hairPath.AppendChild('HairDrySurface')\
                                      .AppendProperty("outputs:surface")
            surfaceTerminal = hairMaterial.GetSurfaceOutput()
            self.assertTrue(surfaceTerminal.HasConnectedSource())
            connectedSurfaceInfo = surfaceTerminal.GetConnectedSources()[0][0]
            
            targetSurfacePath = UsdShade.Utils.GetConnectedSourcePath(connectedSurfaceInfo)
            self.assertEqual(targetSurfacePath, dryHairSurfPath)


            # Validate dry displacement terminal connection
            dryHairDispPath = hairPath.AppendChild('HairDryDisplacement')\
                                      .AppendProperty("outputs:displacement")
            dispTerminal = hairMaterial.GetDisplacementOutput()
            self.assertTrue(dispTerminal.HasConnectedSource())
            connectedDispInfo = dispTerminal.GetConnectedSources()[0][0]
            
            targetDispPath = UsdShade.Utils.GetConnectedSourcePath(connectedDispInfo)
            self.assertEqual(targetDispPath, dryHairDispPath)

        def SetupStage(stage):
            UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
            UsdGeom.SetStageMetersPerUnit(stage, 
                                          UsdGeom.LinearUnits.centimeters)
            
            # Create this prim first, since it's the "entrypoint" to the
            # layer, and we want it to appear at the top
            rootPrim = stage.DefinePrim("/ModelShading")
            stage.SetDefaultPrim(rootPrim)
            return rootPrim

        stage = Usd.Stage.CreateNew("char_shading.usda")
        rootPrim = SetupStage(stage)

        # Next, create a tree that will "sit on top of ShadingDefs" to switch
        # the materials in concert
        allMaterials = SetupShading(stage)
        bindingVariantRoot = stage.OverridePrim("/MaterialBindingVariants")
        UsdShade.Material.CreateMasterMaterialVariant(bindingVariantRoot, allMaterials)

        # Finally, this is how we stitch them together into an interface.
        # This is the root prim that a client would target to pull in shading
        refs = rootPrim.GetReferences()
        refs.AddInternalReference("/MaterialBindingVariants")
        refs.AddInternalReference(shadingDefs)

        stage.Save()

        # Now let's do some validation that it performs as expected
        ValidateMaterial(stage)


        # Now let's make a variation of the above in which we do the master
        # variant on a composed stage in which rootPrim is already
        # referencing the ShadingDefs, and operating on the composed Material
        # prims
        stage = Usd.Stage.CreateNew("char_shading_compact.usda")
        rootPrim = SetupStage(stage)

        # Ignore the return value, since we do not process the materials in
        # their authored namespace, in this version
        SetupShading(stage)
        # Reference the shading directly
        refs = rootPrim.GetReferences()
        refs.AddInternalReference(shadingDefs)

        # Now pick up the newly composed material prims
        allMaterials = [ stage.GetPrimAtPath("/ModelShading/Materials/HairMaterial"),
                     stage.GetPrimAtPath("/ModelShading/Materials/SkinMaterial"),
                     stage.GetPrimAtPath("/ModelShading/Materials/NailsMaterial") ]

        UsdShade.Material.CreateMasterMaterialVariant(rootPrim, allMaterials)

        stage.Save()

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
