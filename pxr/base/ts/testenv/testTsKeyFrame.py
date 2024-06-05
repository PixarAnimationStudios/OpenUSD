#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import sys
from pxr import Ts, Tf, Gf

default_knotType = Ts.KnotLinear
EPSILON = 1e-6

########################################################################

print('\nTest creating keyframe with float time')
kf1_time = 0
kf1 = Ts.KeyFrame( kf1_time )
assert kf1
assert kf1.time == kf1_time
assert kf1.value == 0.0
assert kf1.knotType == default_knotType
assert not kf1.isDualValued
assert eval(repr(kf1)) == kf1
print('\tPassed')

########################################################################

print('\nTest creating 2nd keyframe')
kf2_time = 20
kf2 = Ts.KeyFrame( kf2_time )
assert kf2
assert kf2.time == kf2_time
assert kf2.value == 0.0
assert kf2.knotType == default_knotType
assert not kf2.isDualValued
assert eval(repr(kf2)) == kf2
print('\tPassed')

########################################################################

print('\nTest creating keyframe with keyword arguments')
kf3_time = 30
kf3_value = 0.1234
kf3_knotType = Ts.KnotBezier
kf3_leftLen = 0.1
kf3_leftSlope = 0.2
kf3_rightLen = 0.3
kf3_rightSlope = 0.4
kf3 = Ts.KeyFrame( kf3_time, value = kf3_value, knotType = kf3_knotType,
    leftLen = kf3_leftLen, leftSlope = kf3_leftSlope,
    rightLen = kf3_rightLen, rightSlope = kf3_rightSlope )
assert kf3
assert kf3.time == kf3_time
assert Gf.IsClose(kf3.value, kf3_value, EPSILON)
assert kf3.knotType == kf3_knotType
assert kf3.supportsTangents
assert Gf.IsClose(kf3.leftLen, kf3_leftLen, EPSILON)
assert Gf.IsClose(kf3.leftSlope, kf3_leftSlope, EPSILON)
assert Gf.IsClose(kf3.rightLen, kf3_rightLen, EPSILON)
assert Gf.IsClose(kf3.rightSlope, kf3_rightSlope, EPSILON)
assert eval(repr(kf3)) == kf3
print('\tPassed')

########################################################################

print('\nTest creating dual-valued keyframe')
kf4_time = 40
kf4_value = 0.1234
kf4_knotType = Ts.KnotHeld
kf4_leftLen = 0.1
kf4_leftSlope = 0.2
kf4_rightLen = 0.3
kf4_rightSlope = 0.4
kf4 = Ts.KeyFrame( kf4_time, value = kf4_value, knotType = kf4_knotType,
    leftLen = kf4_leftLen, leftSlope = kf4_leftSlope,
    rightLen = kf4_rightLen, rightSlope = kf4_rightSlope )
assert kf4
assert kf4.time == kf4_time
assert Gf.IsClose(kf4.value, kf4_value, EPSILON)
assert kf4.knotType == kf4_knotType
assert Gf.IsClose(kf4.leftLen, kf4_leftLen, EPSILON)
assert Gf.IsClose(kf4.leftSlope, kf4_leftSlope, EPSILON)
assert Gf.IsClose(kf4.rightLen, kf4_rightLen, EPSILON)
assert Gf.IsClose(kf4.rightSlope, kf4_rightSlope, EPSILON)
assert not kf4.isDualValued
assert eval(repr(kf4)) == kf4

kf4.value = ( 1.234, 4.567 )
assert kf4.isDualValued
# Setting to a tuple of the wrong size should fail

assert eval(repr(kf4)) == kf4

try:
    kf4.value = ( 1.234, )
    assert 0, "should not have worked"
except ValueError:
    pass
try:
    kf4.value = ( 1., 2., 3. )
    assert 0, "should not have worked"
except ValueError:
    pass
# Check that setting a single side of a dual-valued knot works
kf4.value = 999.
assert Gf.IsClose(kf4.value, (1.234, 999.), EPSILON)
print('\tPassed')

print('\nTest convenience get/set value API:')
kf4.SetValue(2.345, Ts.Left)
assert eval(repr(kf4)) == kf4
kf4.SetValue(3.456, Ts.Right)
assert eval(repr(kf4)) == kf4
assert Gf.IsClose(kf4.value, (2.345, 3.456), EPSILON)
assert Gf.IsClose(kf4.GetValue(Ts.Left), 2.345, EPSILON)
assert Gf.IsClose(kf4.GetValue(Ts.Right), 3.456, EPSILON)

print('\nTest that setting bad type on left value does not work: ')
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
oldVal = kf4.value
badVal = ('foo', oldVal[1])
assert badVal != oldVal
assert kf4.value == oldVal
try:
    kf4.value = badVal
    assert False, "should have failed"
except Tf.ErrorException:
    pass
assert kf4.value == oldVal
assert kf4.value != badVal
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
print('\tPassed')

print("\nTest that single -> dual mirrors the signle value to both sides")
val1 = 1.23
val2 = 4.56
kf = Ts.KeyFrame( 0.0, val1 )
kf.isDualValued = True
assert kf.value == (val1, val1)
kf.value = val2
assert kf.value == (val1, val2)
kf.isDualValued = False
kf.isDualValued = True
assert kf.value == (val2, val2)
assert eval(repr(kf)) == kf
print('\tPassed')

########################################################################

print('\nTest non-interpolatable types: errors expected')

testValues = ['string_value', False]

print('\n=== EXPECTED ERRORS ===', file=sys.stderr)

for testValue in testValues:

    print('\t', type(testValue))

    # Test creating keyframes
    kf5_time = 50
    kf5_value = testValue
    kf5_knotType = Ts.KnotHeld
    kf5 = Ts.KeyFrame( kf5_time, kf5_value, knotType = Ts.KnotHeld )

    assert eval(repr(kf5)) == kf5

    assert kf5
    assert kf5.time == kf5_time
    assert kf5.value == kf5_value
    assert kf5.knotType == kf5_knotType
    assert not kf5.isDualValued

    # Test that they can't be dual-valued
    assert not kf5.isDualValued
    try:
        kf5.isDualValued = True
    except Tf.ErrorException:
        pass
    else:
        assert False
    try:
        kf5.value = ('left', 'right')
        assert False, "should have failed"
    except TypeError:
        assert not kf5.isDualValued
        assert kf5.value == kf5_value
        pass

    # Test that they can only be Held
    assert kf5.knotType == Ts.KnotHeld
    try:
        kf5.knotType = Ts.KnotLinear
        assert False, "should have failed"
    except Tf.ErrorException:
        pass
    assert kf5.knotType == Ts.KnotHeld

    # Test that they can't have tangents
    try:
        Ts.KeyFrame(0, kf5_value, kf5_value, \
            Ts.KnotBezier, kf5_value, kf5_value, 0, 0)
    except Tf.ErrorException:
        pass

# Test that non-interpolatable types only set held knots
kf_bool = Ts.KeyFrame( 0.0, True, knotType = Ts.KnotLinear )
assert kf_bool.knotType is Ts.KnotHeld
assert eval(repr(kf_bool)) == kf_bool
kf_bool = Ts.KeyFrame( 0.0, True, knotType = Ts.KnotBezier )
assert kf_bool.knotType is Ts.KnotHeld
assert eval(repr(kf_bool)) == kf_bool
del kf_bool

# Test that interpolatable types w/o tangents only set linear knots
kf_matrix = Ts.KeyFrame( 0.0, Gf.Matrix4d(), knotType = Ts.KnotBezier )
assert eval(repr(kf_matrix)) == kf_matrix
assert kf_matrix.knotType is Ts.KnotLinear
del kf_matrix

print('=== END EXPECTED ERRORS ===', file=sys.stderr)

print('\tPassed')

########################################################################

print('\nTest keyframe equality / inequality')
expected = [kf1, kf2, kf3, kf4, kf5]
for i in range(len(expected)):
    for j in range(i, len(expected)):
        if i == j:
            assert expected[i] == expected[j]
        else:
            assert expected[i] != expected[j]
print('\tPassed')

########################################################################

print('\nTest creating keyframe of bogus type')
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
try:
    # create a keyframe out of the Ts python module
    Ts.KeyFrame( 0, Ts )
    assert False, 'should have failed'
except Tf.ErrorException:
    pass
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
print('\tPassed')

print('\nTest that you cannot change the type of a keyframe: errors expected')
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
testVal = 0.123
kf = Ts.KeyFrame( 0, testVal )
assert kf.value == testVal
try:
    kf.value = 'foo'
    assert False, "expected failure"
except Tf.ErrorException:
    pass
assert kf.value == testVal
del testVal
del kf
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
print('\tPassed')

print('\nTest that you cannot set a keyframe to a non-Vt.Value: ' \
    'errors expected')
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
kf = Ts.KeyFrame( 0, 123.0 )
testVal = 0.123
kf = Ts.KeyFrame( 0, testVal )
assert kf.value == testVal
try:
    kf.value = Gf  # can't store a python module as a Vt.Value
    assert False, "should have failed"
except Tf.ErrorException:
    pass
assert kf.value == testVal
del testVal
del kf
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
print('\tPassed')

########################################################################

print('\nTest that string-value knots do not have tangents: errors expected')
kf = Ts.KeyFrame( 0, 'foo', Ts.KnotHeld)
assert not kf.supportsTangents
# Do the code coverage dance
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
try:
    kf.leftLen = 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    kf.rightLen = 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    kf.leftSlope = 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    kf.rightSlope = 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    assert kf.leftLen == 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    assert kf.rightLen == 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    assert kf.leftSlope.empty
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    assert kf.rightSlope.empty
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    kf.tangentSymmetryBroken = 0
    assert False, "should have failed"
except Tf.ErrorException:
    pass
try:
    assert not kf.tangentSymmetryBroken
    assert False, "should have failed"
except Tf.ErrorException:
    pass
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
print('\tPassed')

print('\nTest that setting tangents to value of wrong type does not work: ' \
    'errors expected')
kf = Ts.KeyFrame( 0, 123.0 )
assert kf.supportsTangents
assert kf.leftSlope == 0
print('\n=== EXPECTED ERRORS ===', file=sys.stderr)
try:
    kf.leftSlope = 'foo'
    assert False, "should have failed"
except Tf.ErrorException:
    pass
assert kf.leftSlope == 0
assert kf.rightSlope == 0
try:
    kf.rightSlope = 'foo'
    assert False, "should have failed"
except Tf.ErrorException:
    pass
assert kf.rightSlope == 0
print('=== END EXPECTED ERRORS ===', file=sys.stderr)
assert eval(repr(kf)) == kf
print('\tPassed')

print('\nTest tangent interface')
target_time = 1.2
target_value = 3.4
kf = Ts.KeyFrame( target_time, target_value )
target_leftLen = 0.1
target_leftSlope = 0.2
target_rightLen = 0.3
target_rightSlope = 0.4
kf.leftLen = target_leftLen
assert eval(repr(kf)) == kf
kf.rightLen = target_rightLen
assert eval(repr(kf)) == kf
kf.leftSlope = target_leftSlope
assert eval(repr(kf)) == kf
kf.rightSlope = target_rightSlope
assert eval(repr(kf)) == kf
assert Gf.IsClose(kf.leftLen, target_leftLen, EPSILON)
assert Gf.IsClose(kf.leftSlope, target_leftSlope, EPSILON)
assert Gf.IsClose(kf.rightLen, target_rightLen, EPSILON)
assert Gf.IsClose(kf.rightSlope, target_rightSlope, EPSILON)
kf.knotType = Ts.KnotBezier
kf.tangentSymmetryBroken = 1
assert kf.tangentSymmetryBroken
kf.tangentSymmetryBroken = 0
assert not kf.tangentSymmetryBroken
assert eval(repr(kf)) == kf
print('\tPassed')

print('\nTest whether the test for having tangents works')
kft_time = 30
kft_value = 0.1234
kft_knotType = Ts.KnotBezier
kft_leftLen = 0.1
kft_leftSlope = 0.2
kft_rightLen = 0.3
kft_rightSlope = 0.4
kft = Ts.KeyFrame( kft_time, value = kft_value, knotType = kft_knotType,
    leftLen = kft_leftLen, leftSlope = kft_leftSlope,
    rightLen = kft_rightLen, rightSlope = kft_rightSlope )
assert eval(repr(kft)) == kft

kfnt_knotType = Ts.KnotHeld
kfnt = Ts.KeyFrame( kft_time, value = kft_value, knotType = kfnt_knotType,
    leftLen = kft_leftLen, leftSlope = kft_leftSlope,
    rightLen = kft_rightLen, rightSlope = kft_rightSlope )
assert eval(repr(kfnt)) == kfnt

assert kft.hasTangents
assert not kfnt.hasTangents
print('\tPassed')

print('\nTest keyframe ordering')
knots = [kf1, kf2, kf3, kf4, kf5]
for i in range(len(knots)):
    assert knots[i].time == knots[i].time
    assert knots[i].time <= knots[i].time
    assert not (knots[i].time < knots[i].time)
    assert not (knots[i].time > knots[i].time)
    for j in range(i+1, len(knots)):
        assert knots[i].time != knots[j].time
        assert knots[i].time < knots[j].time
        assert not (knots[j].time < knots[i].time)
print('\tPassed')

########################################################################

print('\nTest CanSetKnotType')

# Types like doubles support all knot types.
kf = Ts.KeyFrame(0.0, 0.0, Ts.KnotHeld)
assert kf.CanSetKnotType(Ts.KnotBezier)
assert kf.CanSetKnotType(Ts.KnotHeld)
assert kf.CanSetKnotType(Ts.KnotLinear)
assert eval(repr(kf)) == kf

# Some types do not support tangents, but are interpolatable.
kf = Ts.KeyFrame(0.0, Gf.Vec3d(), Ts.KnotHeld)
assert not kf.CanSetKnotType(Ts.KnotBezier)
assert kf.CanSetKnotType(Ts.KnotHeld)
assert kf.CanSetKnotType(Ts.KnotLinear)
assert eval(repr(kf)) == kf

# Some types do not support interpolation or tangents.
kf = Ts.KeyFrame(0.0, "foo", Ts.KnotHeld)
assert not kf.CanSetKnotType(Ts.KnotBezier)
assert kf.CanSetKnotType(Ts.KnotHeld)
assert not kf.CanSetKnotType(Ts.KnotLinear)
assert eval(repr(kf)) == kf

print('\tPassed')

########################################################################

print('\nTest Side based equivalence')

#Control knot 
kf_time = 5
kf_value = 10.0
kf_knotType = Ts.KnotBezier
kf_leftSlope = 0.5
kf_leftLen = 1
kf_rightSlope = 0.7
kf_rightLen = 3
kf = Ts.KeyFrame(kf_time, kf_value, kf_knotType, kf_leftSlope, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert eval(repr(kf)) == kf

#Same knot (equivalent both sides)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_knotType, kf_leftSlope, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert kf == kfNew
assert kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different time (not equivalent)
kfNew = Ts.KeyFrame(20, kf_value, kf_knotType, kf_leftSlope, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert kf.time != kfNew.time
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different value (not equivalent)
kfNew = Ts.KeyFrame(kf_time, 20, kf_knotType, kf_leftSlope, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert kf.value != kfNew.value
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different knot type (not equivalent)
kfNew = Ts.KeyFrame(kf_time, kf_value, Ts.KnotLinear, kf_leftSlope, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert kf.knotType != kfNew.knotType
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different left tangent slope (right equivalent)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_knotType, 0.25, kf_rightSlope,
     kf_leftLen, kf_rightLen)
assert kf.leftSlope != kfNew.leftSlope
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different left tangent length (right equivalent)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_knotType, kf_leftSlope, kf_rightSlope,
     2, kf_rightLen)
assert kf.leftLen != kfNew.leftLen
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different right tangent slope (left equivalent)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_knotType, kf_leftSlope, 0.25,
     kf_leftLen, kf_rightLen)
assert kf.rightSlope != kfNew.rightSlope
assert kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Same knot with a different right tangent length (left equivalent)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_knotType, kf_leftSlope, kf_rightSlope,
     kf_leftLen, 2)
assert kf.rightLen != kfNew.rightLen
assert kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Dual valued knots (Both sides equal)
kfNew = Ts.KeyFrame(kf_time, kf_value, kf_value, kf_knotType, kf_leftSlope, 
    kf_rightSlope, kf_leftLen, kf_rightLen)
assert kfNew.isDualValued
assert kf.GetValue(Ts.Left) == kfNew.GetValue(Ts.Left)
assert kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.GetValue(Ts.Right) == kfNew.GetValue(Ts.Right)
assert kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Dual valued knots (Left side equal)
kfNew = Ts.KeyFrame(kf_time, kf_value, 15.0, kf_knotType, kf_leftSlope, 
    kf_rightSlope, kf_leftLen, kf_rightLen)
assert kfNew.isDualValued
assert kf.GetValue(Ts.Left) == kfNew.GetValue(Ts.Left)
assert kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.GetValue(Ts.Right) != kfNew.GetValue(Ts.Right)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Right)

#Dual valued knots (Right side equal)
kfNew = Ts.KeyFrame(kf_time, 15.0, kf_value, kf_knotType, kf_leftSlope, 
    kf_rightSlope, kf_leftLen, kf_rightLen)
assert kfNew.isDualValued
assert kf.GetValue(Ts.Left) != kfNew.GetValue(Ts.Left)
assert not kf.IsEquivalentAtSide(kfNew, Ts.Left)
assert kf.GetValue(Ts.Right) == kfNew.GetValue(Ts.Right)
assert kf.IsEquivalentAtSide(kfNew, Ts.Right)

print('\tPassed')

########################################################################
print('\nTest eval(repr(...))')
kf = Ts.KeyFrame(0.7, 15.0, Ts.KnotBezier,
                    leftSlope = 1.0, rightSlope = 2.0,
                    leftLen = 1.2, rightLen = 3.4)

# Test single and dual valued
assert eval(repr(kf)) == kf
kf.value = (3.4,5.6)
assert eval(repr(kf)) == kf

########################################################################
# 
print('\nTest SUCCEEDED')
