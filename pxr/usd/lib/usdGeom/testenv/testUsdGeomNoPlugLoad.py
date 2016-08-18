#!/pxrpythonsubst

# This test explicitly does *not* load usdGeom.  We're testing that core Usd
# functionality will work using only information loaded from usdGeom's
# plugInfo.json and generated schema, without loading the library.

from pxr import Plug, Usd

# Find 'UsdGeomScope' by name, assert its size is unknown to TfType (this is
# true since the library is not loaded and the type is only Declare()'d not
# Define()'d.
plugReg = Plug.Registry()
scopeType = plugReg.FindDerivedTypeByName(Usd.Typed, 'UsdGeomScope')

assert scopeType
assert scopeType.sizeof == 0, 'Expected Declared() but not Defined() type'

# Make a stage with a Scope, ensure we can type-check it.
stage = Usd.Stage.CreateInMemory()
scope = stage.DefinePrim('/scope', 'Scope')

assert scope
assert scope.IsA(Usd.Typed)

# Now ensure that fallbacks from generated schema apply without loading plugins.
# Create a Cube prim and assert that size appears as an attribute with a
# fallback value, even though there is no scene description for it.
cube = stage.DefinePrim('/cube', 'Cube')
assert cube

# Unauthored builtins should be reported by GetAttributes and GetProperties.
assert 'size' in [attr.GetName() for attr in cube.GetAttributes()]
assert 'size' in [prop.GetName() for prop in cube.GetProperties()]
assert 'size' in cube.GetPropertyNames()

# ... but should not appear for authored-only queries.
assert 'size' not in [attr.GetName() for attr in cube.GetAuthoredAttributes()]
assert 'size' not in [prop.GetName() for prop in cube.GetAuthoredProperties()]
assert 'size' not in cube.GetAuthoredPropertyNames()

# Fallback values should come from definition.
sizeAttr = cube.GetAttribute('size')
assert sizeAttr
assert sizeAttr.Get() == 2.0
assert not sizeAttr.IsAuthored()

