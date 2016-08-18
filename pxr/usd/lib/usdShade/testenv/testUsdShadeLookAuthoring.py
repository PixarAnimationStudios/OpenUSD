#!/pxrpythonsubst

from pxr import Sdf, Usd, UsdGeom, UsdShade

from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

# There are a number of ways we could vary shading between wet and dry...
# We're choosing the biggest hammer, which is to completely swap out
# the surface shader (which is how it has been done in our pipeline)
shadeVariations = ["Wet", "Dry"]

# For each "base" we will have a Look, and a single Shader as the surface.
# In reality there'd likely be many more shading components/prims feeding
# the surfaces.
lookBases = ["Hair", "Skin", "Nails"]

shadersPath = Sdf.Path("/ShadingDefs/Shaders")
looksPath   = Sdf.Path("/ShadingDefs/Looks")

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

def MakeLookPath(base, prop = None):
    retval = looksPath.AppendChild(base + "Look")
    if prop:
        retval = retval.AppendProperty(prop)
    return retval

def SetupShading(stage):
    # First create the shading prims
    UsdGeom.Scope.Define(stage, shadersPath)
    UsdGeom.Scope.Define(stage, looksPath)
    # .. and as we create each surface, bind the associated look's variant to it
    allLooks = []
    for look in lookBases:
        lookPrim = UsdShade.Look.Define(stage, MakeLookPath(look))
        allLooks.append(lookPrim.GetPrim())
        for variant in shadeVariations:
            surface = UsdShade.Shader.Define(
                stage, MakeSurfacePath(look, variant))
            disp = UsdShade.Shader.Define(
                stage, MakeDisplacementPath(look, variant))
            pattern = UsdShade.Shader.Define(
                stage, MakePatternPath(look, variant))

            with lookPrim.GetEditContextForVariant(variant):
                surfRel = lookPrim.CreateSurfaceTerminal(
                    MakeSurfacePath(look, variant, 'out'))
                dispRel = lookPrim.CreateDisplacementTerminal(
                    MakeDisplacementPath(look, variant, 'out'))
                # this is a subgraph feature
                patternRel = lookPrim.CreateTerminal(
                    'pattern', MakePatternPath(look, variant, 'out'))

    return allLooks

def ValidateLook(stage):
    hairPrim = stage.GetPrimAtPath('/ModelShading/Looks/HairLook')
    hairLook = UsdShade.Look(hairPrim)
    assert hairLook
    assert hairLook.GetLookVariant().GetVariantSelection() == "Wet"
    wetHairSurfPath = Sdf.Path('/ModelShading/Shaders/HairWetSurface.out')
    wetHairDispPath = Sdf.Path('/ModelShading/Shaders/HairWetDisp.out')
    wetHairPatternPath = Sdf.Path('/ModelShading/Shaders/HairWetPattern.out')
    assert hairLook.GetSurfaceTerminal().GetTargets()[0] == wetHairSurfPath
    assert hairLook.GetDisplacementTerminal().GetTargets()[0] == wetHairDispPath
    assert hairLook.GetTerminal('pattern').GetTargets()[0] == wetHairPatternPath
    # change the root-level variantSet, which should in turn change the Look's
    assert rootPrim.GetVariantSets().SetSelection("lookVariant", "Dry")
    assert hairLook
    dryHairSurfPath = Sdf.Path('/ModelShading/Shaders/HairDrySurface.out')
    dryHairDispPath = Sdf.Path('/ModelShading/Shaders/HairDryDisp.out')
    dryHairPatternPath = Sdf.Path('/ModelShading/Shaders/HairDryPattern.out')
    assert hairLook.GetSurfaceTerminal().GetTargets()[0] == dryHairSurfPath
    assert hairLook.GetDisplacementTerminal().GetTargets()[0] == dryHairDispPath
    assert hairLook.GetTerminal('pattern').GetTargets()[0] == dryHairPatternPath


fileName = "char_shading.usda"
stage = Usd.Stage.CreateNew(fileName)

# Create this prim first, since it's the "entrypoint" to the layer, and
# we want it to appear at the top
rootPrim = stage.DefinePrim("/ModelShading")

# Next, create a tree that will "sit on top of ShadingDefs" to switch the
# looks in concert
allLooks = SetupShading(stage)
bindingVariantRoot = stage.OverridePrim("/LookBindingVariants")
UsdShade.Look.CreateMasterLookVariant(bindingVariantRoot, allLooks)

# Finally, this is how we stitch them together into an interface.
# This is the root prim that a client would target to pull in shading
refs = rootPrim.GetReferences()
# XXX We need a better way of specifying self-references
refs.Add("./"+fileName, "/LookBindingVariants")
refs.Add("./"+fileName, "/ShadingDefs")

stage.GetRootLayer().Save()

# Now let's do some validation that it performs as expected
ValidateLook(stage)



# Now let's make a variation of the above in which we do the master variant
# on a composed stage in which rootPrim is already referencing the ShadingDefs,
# and operating on the composed Look prims
fileName = "char_shading_compact.usda"
stage = Usd.Stage.CreateNew(fileName)

# Create this prim first, since it's the "entrypoint" to the layer, and
# we want it to appear at the top
rootPrim = stage.DefinePrim("/ModelShading")

SetupShading(stage)
# Reference the shading directly
refs = rootPrim.GetReferences()
refs.Add("./"+fileName, "/ShadingDefs")

# Now pick up the newly composed look prims
allLooks = [ stage.GetPrimAtPath("/ModelShading/Looks/HairLook"),
             stage.GetPrimAtPath("/ModelShading/Looks/SkinLook"),
             stage.GetPrimAtPath("/ModelShading/Looks/NailsLook") ]

UsdShade.Look.CreateMasterLookVariant(rootPrim, allLooks)

stage.GetRootLayer().Save()

# Now let's do some validation that it performs as expected
ValidateLook(stage)

# TODO: move this into it's own test
hairPrim = stage.GetPrimAtPath('/ModelShading/Looks/HairLook')
hairLook = UsdShade.Look(hairPrim)
interfaceAttribute = hairLook.CreateInterfaceAttribute("myParam", Sdf.ValueTypeNames.Float)
interfaceAttribute.SetDocumentation("this is my float")
interfaceAttribute.SetDisplayGroup("numbers")

# Make sure the IA is namespaced, but also that we can retrieve it with or
# without the namespace
plain = hairPrim.GetAttribute("interface:myParam")
assert plain and ( plain.GetTypeName() == "float" )

# plain was a UsdAttribue, but these are UsdShadeInterfaceAttributes
byName = hairLook.GetInterfaceAttribute("myParam")
assert byName and ( byName.GetDocumentation() == "this is my float" )

fullName = hairLook.GetInterfaceAttribute("interface:myParam")
assert fullName and ( fullName.GetDisplayGroup() == "numbers" )
