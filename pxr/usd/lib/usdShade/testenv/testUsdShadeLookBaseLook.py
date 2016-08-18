#!/pxrpythonsubst

from pxr import Tf, Sdf, Usd, UsdGeom, UsdShade

from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

stage = Usd.Stage.CreateInMemory()

UsdGeom.Scope.Define(stage, "/Model")
UsdGeom.Scope.Define(stage, "/Model/Looks")

# ===================================
print ("Test basic relationship APIs")
# ===================================
# Create some looks
grandParent = UsdShade.Look.Define(stage, "/Model/Looks/GrandParent")
parent = UsdShade.Look.Define(stage, "/Model/Looks/GrandParent_Parent")
child = UsdShade.Look.Define(stage, "/Model/Looks/GrandParent_Parent_Child")
assert grandParent
assert parent
assert child

# Set base looks
parent.SetBaseLook(grandParent)
child.SetBaseLookPath(parent.GetPrim().GetPath())

assert parent.GetBaseLook().GetPrim() == grandParent.GetPrim()
assert parent.GetBaseLookPath() == grandParent.GetPrim().GetPath()
assert child.GetBaseLook().GetPrim() == parent.GetPrim()
assert child.GetBaseLookPath() == parent.GetPrim().GetPath()

# Create shaders:
# Look GrandParent
#   Shader ShaderA
# Look Parent (base Look: GrandParent)
#   Shader ShaderA (derives from GrandParent)
#   Shader ShaderB
# Look Child (base Look: GrandParent)
#   Shader ShaderA (derives from Parent:GrandParent)
#   Shader ShaderB (derives from Parent)
#   Shader ShaderC

grandParentShaderA = UsdShade.Shader.Define(stage,
        grandParent.GetPrim().GetPath().AppendChild("ShaderA"))
parentShaderA = UsdShade.Shader.Define(stage,
        parent.GetPrim().GetPath().AppendChild("ShaderA"))
parentShaderB = UsdShade.Shader.Define(stage,
        parent.GetPrim().GetPath().AppendChild("ShaderB"))
childShaderA = UsdShade.Shader.Define(stage,
        child.GetPrim().GetPath().AppendChild("ShaderA"))
childShaderC = UsdShade.Shader.Define(stage,
        child.GetPrim().GetPath().AppendChild("ShaderC"))
childShaderB = UsdShade.Shader.Define(stage,
        child.GetPrim().GetPath().AppendChild("ShaderB"))
assert grandParentShaderA
assert parentShaderA
assert parentShaderB
assert childShaderA
assert childShaderB
assert childShaderC


# =================================
print ("Test parameter inheritance")
# =================================

def getComposedParameter(look, shaderName, paramName):
    ''' Manually "composes" shader parameters and returns the
    requested parameter value. This should be taken care of the derivesFrom
    Usd composition arc, once it's ready. This function is a placeholder
    until the composition arc functionality exists.'''
    # TODO: Maybe accept shader instead of shaderName? We will need a link
    # from shader to look
    lookPath = look.GetPath()
    shaderPath = lookPath.AppendChild(shaderName)
    shaderPrim = look.GetPrim().GetStage().GetPrimAtPath(shaderPath)
    if shaderPrim.IsValid():
        shader = UsdShade.Shader(shaderPrim)
        localParams = shader.GetParameters()
        for param in localParams:
            if param.GetAttr().GetName() == paramName:
                return param

    if look.HasBaseLook():
        # Find the shader's
        return getComposedParameter(look.GetBaseLook(), shaderName, paramName)

    return None

# Set parameters:
# Look GrandParent
#   Shader ShaderA
#       paramA = grandParentA
#       paramB = grandParentB
#       paramC = grandParentC
#
# Look Parent (base Look: GrandParent)
#   Shader ShaderA (derives from GrandParent)
#       paramA = grandParentA (derives from GrandParent)
#       paramB = grandParentB (derives from GrandParent)
#       paramC = parentC (derives from GrandParent)
#   Shader ShaderB
#       paramA = parentA
#       paramB = parentB
#
# Look Child (base Look: GrandParent)
#   Shader ShaderA (derives from Parent:GrandParent)
#       paramA = childA
#       paramB = grandParentB (derives from Parent:GrandParent)
#       paramC = parentC (derives from Parent)
#   Shader ShaderB (derives from Parent)
#       paramA = parentA (derives from Parent)
#       paramB = childB
#       paramC = childC
#   Shader ShaderC
#       paramA = childA

grandParentShaderA.CreateParameter("paramA", Sdf.ValueTypeNames.String).Set(
        "grandParentA")
grandParentShaderA.CreateParameter("paramB", Sdf.ValueTypeNames.String).Set(
        "grandParentB")
grandParentShaderA.CreateParameter("paramC", Sdf.ValueTypeNames.String).Set(
        "grandParentC")

parentShaderA.CreateParameter("paramC",
        Sdf.ValueTypeNames.String).Set("parentC")
parentShaderB.CreateParameter("paramA",
        Sdf.ValueTypeNames.String).Set("parentA")
parentShaderB.CreateParameter("paramB",
        Sdf.ValueTypeNames.String).Set("parentB")

childShaderA.CreateParameter("paramA", Sdf.ValueTypeNames.String).Set(
        "childA")
childShaderB.CreateParameter("paramB", Sdf.ValueTypeNames.String).Set(
        "childB")
childShaderB.CreateParameter("paramC", Sdf.ValueTypeNames.String).Set(
        "childC")
childShaderC.CreateParameter("paramA", Sdf.ValueTypeNames.String).Set("childA")


# Basic parameter value tests
assert (grandParentShaderA.GetParameter("paramA").GetAttr().Get()
        == "grandParentA")
assert (grandParentShaderA.GetParameter("paramB").GetAttr().Get()
        == "grandParentB")
assert (grandParentShaderA.GetParameter("paramC").GetAttr().Get()
        == "grandParentC")

assert (parentShaderA.GetParameter("paramC").GetAttr().Get()
        == "parentC")
assert parentShaderB.GetParameter("paramA").GetAttr().Get() == "parentA"
assert parentShaderB.GetParameter("paramB").GetAttr().Get() == "parentB"

assert childShaderC.GetParameter("paramA").GetAttr().Get() == "childA"

assert childShaderA.GetParameter("paramA").GetAttr().Get() == "childA"
assert childShaderB.GetParameter("paramB").GetAttr().Get() == "childB"

# derives parameter value tests
assert (getComposedParameter(grandParent, "ShaderA", "paramA").GetAttr().Get()
        == "grandParentA")
assert (getComposedParameter(grandParent, "ShaderA", "paramB").GetAttr().Get()
        == "grandParentB")
assert (getComposedParameter(grandParent, "ShaderA", "paramC").GetAttr().Get()
        == "grandParentC")

assert (getComposedParameter(parent, "ShaderA", "paramA").GetAttr().Get()
        == "grandParentA")
assert (getComposedParameter(parent, "ShaderA", "paramB").GetAttr().Get()
        == "grandParentB")
assert (getComposedParameter(parent, "ShaderA", "paramC").GetAttr().Get()
        == "parentC")
assert (getComposedParameter(parent, "ShaderB", "paramA").GetAttr().Get()
        == "parentA")
assert (getComposedParameter(parent, "ShaderB", "paramB").GetAttr().Get()
        == "parentB")

assert (getComposedParameter(child, "ShaderA", "paramA").GetAttr().Get()
        == "childA")
assert (getComposedParameter(child, "ShaderA", "paramB").GetAttr().Get()
        == "grandParentB")
assert (getComposedParameter(child, "ShaderA", "paramC").GetAttr().Get()
        == "parentC")
assert (getComposedParameter(child, "ShaderB", "paramA").GetAttr().Get()
        == "parentA")
assert (getComposedParameter(child, "ShaderB", "paramB").GetAttr().Get()
        == "childB")
assert (getComposedParameter(child, "ShaderB", "paramC").GetAttr().Get()
        == "childC")
assert (getComposedParameter(child, "ShaderC", "paramA").GetAttr().Get()
        == "childA")

# ==============================================
# print ("Test parameter relationship inheritance")
# ==============================================
# TODO
# It currently doesn't make sense to test this because the resolution is
# handled by the client, as the derivesFrom arc doesn't yet exist.
# Eventually, we'll want to add this test when the derivesFrom arc is handled
# by USD

# Connect parameters
# Look Parent:
#   Shader ShaderA (derives from GrandParent)
#       paramA connected to ShaderB paramA
#       paramB connected to ShaderB paramB
#   Shader ShaderB

# Look Child:
#   Shader ShaderA (derives from Parent:GrandParent)
#       paramA connected to ShaderB paramA (derives from Parent)
#       paramB connected to ShaderC paramA
#   Shader ShaderB (derives from Parent)
#   Shader ShaderC
#       paramA connected to ShaderB paramA

# Check:
# Parent ShaderA paramA is connected to ShaderB paramA
# Parent ShaderA paramA is connected to ShaderB paramB
# Child ShaderA paramA is connected to ShaderB paramA
# Child ShaderA paramB is connected to ShaderC paramA
# Child ShaderC paramA is connected to ShaderB paramA

print "Finished"
