#!/pxrpythonsubst

from pxr import Sdf, Usd, UsdShade

from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

s = Usd.Stage.CreateInMemory()
rl = s.GetRootLayer()

# set up so the weaker subtree binds gprim to look1, and
# stronger subtree to look2
lw1 = UsdShade.Look.Define(s, "/weaker/look1")
lw2 = UsdShade.Look.Define(s, "/weaker/look2")
gpw = s.OverridePrim("/weaker/gprim")
lw1.Bind(gpw)
assert UsdShade.Look.GetBindingRel(gpw).GetTargets() == [
    Sdf.Path("/weaker/look1")]

ls1 = UsdShade.Look.Define(s, "/stronger/look1")
ls2 = UsdShade.Look.Define(s, "/stronger/look2")
gps = s.OverridePrim("/stronger/gprim")
ls2.Bind(gps)
assert UsdShade.Look.GetBindingRel(gps).GetTargets() == [
    Sdf.Path("/stronger/look2")]

cr = s.OverridePrim("/composed")

cr.GetReferences().Add(rl.identifier, "/stronger")
cr.GetReferences().Add(rl.identifier, "/weaker")

gpc = s.GetPrimAtPath("/composed/gprim")
lb = UsdShade.Look.GetBindingRel(gpc)

# validate we get look2, the stronger binding
assert lb.GetTargets() == [Sdf.Path("/composed/look2")]

# upon unbinding *in* the stronger site (i.e. "/stronger/gprim"),
# we should still be unbound in the fully composed view
UsdShade.Look.Unbind(gps)
assert lb.GetTargets() == []
# but *clearing* the target on the referenced prim should allow
# the weaker binding to shine through
UsdShade.Look.GetBindingRel(gps).ClearTargets(True)
assert lb.GetTargets() == [Sdf.Path("/composed/look1")]

# Test GetBoundLook() API
def TestGetBoundLook():
    print 'Testing GetBoundLook()'
    stage = Usd.Stage.CreateInMemory()
    look = UsdShade.Look.Define(stage, "/World/Look")
    assert look
    gprim = stage.OverridePrim("/World/Gprim")
    assert gprim

    assert not UsdShade.Look.GetBoundLook(gprim)
    look.Bind(gprim)
    assert UsdShade.Look.GetBoundLook(gprim)

    # Now add one more target to mess things up
    rel = UsdShade.Look.GetBindingRel(gprim)
    rel.AddTarget(Sdf.Path("/World"))
    assert not UsdShade.Look.GetBoundLook(gprim)

def TestBlockingOnOver():
    print 'Ensuring we can block a look binding on a pure over, using Unbind()'
    stage = Usd.Stage.CreateInMemory()
    over = stage.OverridePrim('/World/over')
    look = UsdShade.Look.Define(stage, "/World/Look")
    assert look
    gprim = stage.DefinePrim("/World/gprim")
    
    UsdShade.Look.Unbind(over)
    look.Bind(gprim)
    # This will compose in gprim's binding, but should still be blocked
    over.GetInherits().Add("/World/gprim")
    assert not UsdShade.Look.GetBoundLook(over)
    
TestGetBoundLook()
TestBlockingOnOver()

print "All clear, boss!"
