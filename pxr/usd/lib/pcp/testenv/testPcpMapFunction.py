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

from pxr import Sdf, Pcp
import unittest

testPaths = [
    Sdf.Path(),
    Sdf.Path('/'),
    Sdf.Path('/foo'),
    Sdf.Path('/a/b/c')
    ]

testMapFuncs = []

# Test null function
null = Pcp.MapFunction()
testMapFuncs.append(null)
assert null.isNull
assert not null.isIdentity
assert null.timeOffset == Sdf.LayerOffset()
for path in testPaths:
    assert null.MapSourceToTarget( path ).isEmpty

# Test identity function
identity = Pcp.MapFunction.Identity()
testMapFuncs.append(identity)
assert not identity.isNull
assert identity.isIdentity
assert identity.timeOffset == Sdf.LayerOffset()
for path in testPaths:
    assert identity.MapSourceToTarget( path ) == path

# Test a simple mapping, simulating a referenced model instance.
m = Pcp.MapFunction({'/Model': '/Model_1'})
testMapFuncs.append(m)
assert not m.isNull
assert not m.isIdentity
assert m.MapSourceToTarget('/').isEmpty
assert m.MapSourceToTarget('/Model') == Sdf.Path('/Model_1')
assert m.MapSourceToTarget('/Model/anim') == Sdf.Path('/Model_1/anim')
assert m.MapSourceToTarget('/Model_1').isEmpty
assert m.MapSourceToTarget('/Model_1/anim').isEmpty
assert m.MapTargetToSource('/').isEmpty
assert m.MapTargetToSource('/Model').isEmpty
assert m.MapTargetToSource('/Model/anim').isEmpty
assert m.MapTargetToSource('/Model_1') == Sdf.Path('/Model')
assert m.MapTargetToSource('/Model_1/anim') == Sdf.Path('/Model/anim')

# Mapping functions do not affect nested target paths.
assert (m.MapTargetToSource('/Model_1.x[/Model_1.y]') ==
    Sdf.Path('/Model.x[/Model_1.y]'))
assert (m.MapSourceToTarget('/Model.x[/Model.y]') ==
    Sdf.Path('/Model_1.x[/Model.y]'))

# Test a mapping representing a nested rig reference.
m2 = Pcp.MapFunction({'/CharRig': '/Model/Rig'})
testMapFuncs.append(m2)
assert not m2.isNull
assert not m2.isIdentity
assert m2.MapSourceToTarget('/').isEmpty
assert m2.MapSourceToTarget('/CharRig') == Sdf.Path('/Model/Rig')
assert m2.MapSourceToTarget('/CharRig/rig') == Sdf.Path('/Model/Rig/rig')
assert m2.MapSourceToTarget('/Model').isEmpty
assert m2.MapSourceToTarget('/Model/Rig').isEmpty
assert m2.MapSourceToTarget('/Model/Rig/rig').isEmpty
assert m2.MapTargetToSource('/').isEmpty
assert m2.MapTargetToSource('/CharRig').isEmpty
assert m2.MapTargetToSource('/CharRig/rig').isEmpty
assert m2.MapTargetToSource('/Model').isEmpty
assert m2.MapTargetToSource('/Model/Rig') == Sdf.Path('/CharRig')
assert m2.MapTargetToSource('/Model/Rig/rig') == Sdf.Path('/CharRig/rig')

# Test composing the previous two map functions.
m3 = m.Compose(m2)
testMapFuncs.append(m3)
assert not m3.isNull
assert not m3.isIdentity
assert m3.MapSourceToTarget('/').isEmpty
assert m3.MapSourceToTarget('/CharRig') == Sdf.Path('/Model_1/Rig')
assert m3.MapSourceToTarget('/CharRig/rig') == Sdf.Path('/Model_1/Rig/rig')
assert m3.MapSourceToTarget('/Model').isEmpty
assert m3.MapSourceToTarget('/Model/Rig').isEmpty
assert m3.MapSourceToTarget('/Model/Rig/rig').isEmpty
assert m3.MapSourceToTarget('/Model_1').isEmpty
assert m3.MapSourceToTarget('/Model_1/Rig').isEmpty
assert m3.MapSourceToTarget('/Model_1/Rig/rig').isEmpty
assert m3.MapTargetToSource('/').isEmpty
assert m3.MapTargetToSource('/CharRig').isEmpty
assert m3.MapTargetToSource('/CharRig/rig').isEmpty
assert m3.MapTargetToSource('/Model').isEmpty
assert m3.MapTargetToSource('/Model/Rig').isEmpty
assert m3.MapTargetToSource('/Model/Rig/rig').isEmpty
assert m3.MapTargetToSource('/Model_1').isEmpty
assert m3.MapTargetToSource('/Model_1/Rig') == Sdf.Path('/CharRig')
assert m3.MapTargetToSource('/Model_1/Rig/rig') == Sdf.Path('/CharRig/rig')

# Test a chain of composed mappings that simulates an inherit of
# a local class, along with some relocations:
#
# - M_1 is an instance of the model, M
# - M/Rig/Inst is an instance of the local class M/Rig/Class
# - M/Rig/Inst/Scope is an anim scope relocated to M/Anim/Scope
#
m4 = \
    Pcp.MapFunction( {'/M': '/M_1'} ).Compose(
    Pcp.MapFunction( {'/M/Rig/Inst/Scope': '/M/Anim/Scope'} ).Compose(
    Pcp.MapFunction( {'/M/Rig/Class': '/M/Rig/Inst'} ).Compose(
    Pcp.MapFunction( {'/M_1': '/M'} )
    )))
testMapFuncs.append(m4)

# The composed result should map opinions from the model instance's
# rig class scope, to the relocated anim scope.
expected = Pcp.MapFunction({'/M_1/Rig/Class/Scope': '/M_1/Anim/Scope'})
assert m4 == expected
assert (m4.MapSourceToTarget(
    Sdf.Path('/M_1/Rig/Class/Scope/x')) == Sdf.Path('/M_1/Anim/Scope/x'))
assert (m4.MapTargetToSource(
    Sdf.Path('/M_1/Anim/Scope/x')) == Sdf.Path('/M_1/Rig/Class/Scope/x'))

# Test layer offsets
offset1 = Sdf.LayerOffset(offset=0.0, scale=2.0)
offset2 = Sdf.LayerOffset(offset=10.0, scale=1.0)
m5 = Pcp.MapFunction({'/':'/'}, offset1)
m6 = Pcp.MapFunction({'/':'/'}, offset2)

assert m5.timeOffset == offset1
assert m6.timeOffset == offset2
assert m5.Compose(m6).timeOffset == (offset1 * offset2)

testMapFuncs.append(m5)
testMapFuncs.append(m6)

# Test equality/inequality
for i in range(len(testMapFuncs)):
    for j in range(len(testMapFuncs)):
        if i == j:
            assert testMapFuncs[i] == testMapFuncs[j]
        else:
            assert testMapFuncs[i] != testMapFuncs[j]

# Test repr
for m in testMapFuncs:
    assert eval(repr(m)) == m

# Composing any function with identity should return itself.
for mapFunc in testMapFuncs:
    assert mapFunc.Compose(identity) == mapFunc
    assert identity.Compose(mapFunc) == mapFunc

# Composing a function that has an identity offset with a null function 
# should return null. 
# Composing a function that has an offset with a null function 
# should just return that offset.
for mapFunc in testMapFuncs:
    if mapFunc.timeOffset.IsIdentity():
        assert mapFunc.Compose(null) == null
        assert null.Compose(mapFunc) == null
    else:
        assert mapFunc.Compose(null) == Pcp.MapFunction({}, mapFunc.timeOffset)
        assert null.Compose(mapFunc) == Pcp.MapFunction({}, mapFunc.timeOffset)

# Test map function canonicalization.
assert (Pcp.MapFunction({})
    == Pcp.MapFunction({}))
assert (Pcp.MapFunction({'/A':'/A'})
    == Pcp.MapFunction({'/A':'/A'}))
assert (Pcp.MapFunction({'/':'/', '/A':'/A'})
    == Pcp.MapFunction({'/':'/'}))
assert (Pcp.MapFunction({'/A':'/B', '/A/X': '/B/X', '/A/X/Y1':'/B/X/Y2'}) ==
    Pcp.MapFunction({'/A':'/B', '/A/X/Y1':'/B/X/Y2'}))

# Test case for bug 74847.
def TestBug74847():
    m = Pcp.MapFunction({'/A': '/A/B'})
    assert m.MapSourceToTarget('/A/B') == Sdf.Path('/A/B/B')
    assert m.MapTargetToSource('/A/B/B') == Sdf.Path('/A/B')

TestBug74847()

# Test case for bug 112645. See also TrickyInheritsAndRelocates3 test
# in testPcpMuseum.
def TestBug112645():
    f1 = Pcp.MapFunction(
        {'/GuitarRig':'/GuitarRigX',
         '/GuitarRig/Rig/StringsRig/_Class_StringRig/String':
             '/GuitarRigX/Anim/Strings/String1'})

    f2 = Pcp.MapFunction(
        {'/StringsRig/String1Rig/String' :
             '/GuitarRig/Anim/Strings/String1',
         '/StringsRig' :
             '/GuitarRig/Rig/StringsRig'})

    assert f1.Compose(f2) == Pcp.MapFunction({
            '/StringsRig':'/GuitarRigX/Rig/StringsRig',
            '/StringsRig/_Class_StringRig/String':
                '/GuitarRigX/Anim/Strings/String1'})

TestBug112645()

# TODO:
# test constructing invalid map functions
# test application to anim splines, including animated constraints
