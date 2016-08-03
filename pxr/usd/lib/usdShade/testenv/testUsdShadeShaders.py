#!/pxrpythonsubst

from pxr import Tf, Sdf, Usd, UsdGeom, UsdShade

from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

stage = Usd.Stage.CreateInMemory()

UsdGeom.Scope.Define(stage, "/Model")
UsdGeom.Scope.Define(stage, "/Model/Looks")
UsdShade.Look.Define(stage, "/Model/Looks/LookSharp")

pale = UsdShade.Shader.Define(stage, "/Model/Looks/LookSharp/Pale")
assert pale
whiterPale = UsdShade.Shader.Define(stage, "/Model/Looks/LookSharp/WhiterPale")
assert whiterPale

print ('Test RenderType')
chords = pale.CreateParameter("chords", Sdf.ValueTypeNames.String)
assert chords
assert not chords.HasRenderType()
assert chords.GetRenderType() == ""
chords.SetRenderType("notes")
assert chords.HasRenderType()
assert chords.GetRenderType() == "notes"


def ConnectionsEqual(a, b):
    return a[0].GetPrim() == b[0].GetPrim() and a[1] == b[1]

# Make a class for pale so we can test that disconnecting/blocking works
classPale = stage.CreateClassPrim("/classPale")
assert classPale
pale.GetPrim().GetInherits().Add("/classPale")
shaderClass = UsdShade.Shader(classPale)
# it's not valid because it's not defined, but we can still author using it
assert not shaderClass


################################
print ('Test scalar connections')
################################

usdShadeParam = pale.CreateParameter('myFloatParameter', 
                                     Sdf.ValueTypeNames.Float)
usdShadeParam.Set(1.0)
assert not usdShadeParam.IsConnected()
usdShadeParam.ConnectToSource(whiterPale, 'Fout')
assert usdShadeParam.IsConnected()
assert ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                        (whiterPale, 'Fout'))
usdShadeParam.ClearSources()
assert not usdShadeParam.IsConnected()
assert usdShadeParam.GetConnectedSource() == None
assert usdShadeParam.GetConnectedSources() == None

# Now make the connection in the class
inheritedParam = shaderClass.CreateParameter('myFloatParameter', 
                                             Sdf.ValueTypeNames.Float)
inheritedParam.ConnectToSource(whiterPale, 'Fout')
# note we're now testing the inheritING prim's parameter
assert usdShadeParam.IsConnected()
assert ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                        (whiterPale, 'Fout'))
# Array-queries should all be negative
assert not usdShadeParam.IsArray()
assert usdShadeParam.GetConnectedArraySize() == 0
(sources, outputNames) = usdShadeParam.GetConnectedSources()
assert ConnectionsEqual( (sources[0], outputNames[0]), (whiterPale, 'Fout'))
# clearing no longer changes anything
usdShadeParam.ClearSources()
assert usdShadeParam.IsConnected()
assert ConnectionsEqual(usdShadeParam.GetConnectedSource(),
                        (whiterPale, 'Fout'))
# but disconnecting should
usdShadeParam.DisconnectSources()
assert not usdShadeParam.IsConnected()
assert usdShadeParam.GetConnectedSource() == None


################################
print('Test asset id')
################################
pale.CreateIdAttr('SharedFloat_1')
whiterPale.CreateIdAttr('SharedColor_1')
assert(pale.GetIdAttr().Get() == 'SharedFloat_1')
assert(whiterPale.GetIdAttr().Get() == 'SharedColor_1')

################################
print ('Test array connections')
################################

# First test whole-array connection
array1 = pale.CreateParameter('array1', 
                              Sdf.ValueTypeNames.FloatArray)
assert array1.IsArray()
assert not array1.IsConnected()
assert array1.GetConnectedSources() is None
array1.ConnectToSource(whiterPale, 'otherArray')
assert array1.IsConnected()
assert ConnectionsEqual(array1.GetConnectedSource(),
                        (whiterPale, 'otherArray'))
assert array1.GetConnectedArraySize() == 1
(sources, outputNames) = array1.GetConnectedSources()
assert ConnectionsEqual( (sources[0], outputNames[0]), (whiterPale, 'otherArray'))
# Connect two elements in the class, and verify that didn't change results, 
# because whole-array connection wins
inheritArray1 = shaderClass.CreateParameter('array1', 
                                            Sdf.ValueTypeNames.FloatArray)
inheritArray1.ConnectElementToSource(1, whiterPale, 'presence1')
inheritArray1.ConnectElementToSource(2, whiterPale, 'presence2')
(sources, outputNames) = array1.GetConnectedSources()
assert len(sources) == 1 and len(outputNames) == 1
assert ConnectionsEqual( (sources[0], outputNames[0]), (whiterPale, 'otherArray'))
# When we clear sources on the inheritor, so that the whole-array connection
# goes away, we should then get an array of size three whose first elem
# is not connected, from the class prim
array1.ClearSources()
assert array1.IsConnected()
assert array1.GetConnectedArraySize() == 3
(sources, outputNames) = array1.GetConnectedSources()
assert not sources[0]
assert ConnectionsEqual( (sources[0], outputNames[0]), (UsdShade.Shader(), ''))
assert ConnectionsEqual( (sources[1], outputNames[1]), (whiterPale, 'presence1'))
assert ConnectionsEqual( (sources[2], outputNames[2]), (whiterPale, 'presence2'))
# Explicitly trim the array
array1.SetConnectedArraySize(2)
assert array1.GetConnectedArraySize() == 2
(sources, outputNames) = array1.GetConnectedSources()
assert len(sources) == 2 and len(outputNames) == 2
# Blocking elem 1 should cause the parameter to become completely
# disconnected.  Array size should remain unchanged, however
array1.DisconnectElement(1)
assert not array1.IsConnected()
(sources, outputNames) = array1.GetConnectedSources()
assert len(sources) == 2 and len(outputNames) == 2
assert ConnectionsEqual( (sources[0], outputNames[0]), (UsdShade.Shader(), ''))
assert ConnectionsEqual( (sources[1], outputNames[1]), (UsdShade.Shader(), ''))
# Reasserting the whole-array-connection cause the explicit array length
# to differ from the returned size of GetConnectedSources()
assert array1.GetConnectedSource() is None
array1.ConnectToSource(whiterPale, 'otherArray')
assert array1.IsConnected()
assert array1.GetConnectedArraySize() == 2
(sources, outputNames) = array1.GetConnectedSources()
assert len(sources) == 1 and len(outputNames) == 1


# Test boundaries of parameter type-testing when connecting
print "Test Typing Parameter Connections"

colParam = pale.CreateParameter("col1", Sdf.ValueTypeNames.Color3f);
assert colParam
assert colParam.ConnectToSource(whiterPale, "colorOut")
outputAttr = whiterPale.GetPrim().GetAttribute("outputs:colorOut")
assert outputAttr
assert outputAttr.GetTypeName() == Sdf.ValueTypeNames.Color3f

v3fParam = pale.CreateParameter("v3f1", Sdf.ValueTypeNames.Float3)
assert v3fParam
assert v3fParam.ConnectToSource(whiterPale, "colorOut")

pointParam = pale.CreateParameter("point1", Sdf.ValueTypeNames.Point3f)
assert pointParam
assert pointParam.ConnectToSource(whiterPale, "colorOut")

floatParam = pale.CreateParameter("float1", Sdf.ValueTypeNames.Float)
assert floatParam
# XXX The following test must be disabled until we re-enable strict
# type-checking for parameter connections.  See bug/113600
# can't connect float to color!
#with RequiredException(Tf.ErrorException):
#    floatParam.ConnectToSource(whiterPale, "colorOut")

assert floatParam.ConnectToSource(whiterPale, "floatParam", outputIsParameter=True)
outputAttr = whiterPale.GetPrim().GetAttribute("outputs:floatParam")
assert not outputAttr
outputAttr = whiterPale.GetPrim().GetAttribute("floatParam")
assert outputAttr

print "Test Parameter Fetching"
# test against single param fetches
vecParam = pale.CreateParameter('vec', Sdf.ValueTypeNames.Color3f)
assert vecParam
assert pale.GetParameter('vec')
assert pale.GetParameter('vec').SetRenderType('foo')
assert pale.GetParameter('vec').GetRenderType() == 'foo'

# test against multiple params
params = pale.GetParameters()

# assure new item in collection
assert len([pr for pr in params if pr.GetRenderType() == 'foo']) == 1

# ensure the param count increments properly
oldlen = len(pale.GetParameters())
newparam = pale.CreateParameter('struct', Sdf.ValueTypeNames.Color3f)

# assure new item in collection
assert len(pale.GetParameters()) == (oldlen+1)

# ensure by-value capture in 'params'
assert len(pale.GetParameters()) != len(params)

print "Finale"
