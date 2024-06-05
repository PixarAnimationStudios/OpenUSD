#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function
from pxr import Ts, Vt, Gf, Tf

EPSILON = 1e-6
COARSE_EPSILON = 1e-4


v = Ts.Spline()

def _Validate():
    assert v == eval(repr(v))

########################################################################

print('\nTest basic keyframes interface')

assert len(v) == 0

# Some sample keyframe data.
kf1_time = 0
kf1 = Ts.KeyFrame( kf1_time )
kf2_time = 20
kf2 = Ts.KeyFrame( kf2_time )
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
kf5_time = 50
kf5_value = ''
kf5_knotType = Ts.KnotHeld
kf5 = Ts.KeyFrame( kf5_time, kf5_value, knotType = Ts.KnotHeld )

expected = [kf1, kf2, kf3, kf4]
for kf in expected:
    v.SetKeyFrame(kf)
    assert v
    _Validate()
print(v.frames)
print(expected)
assert len(v.frames) == len(expected)
_Validate()
# Compare the contents of our list to the contents of the proxy
for (kf, time) in zip( expected, v.frames ):
    assert time in v
    assert v[time] == kf
# Test contains
for a in expected:
    assert a.time in v.frames
for b in v:
    assert b in expected
# Test getitem
for a in expected:
    assert v[ a.time ] == a
# Test accessing keyframe at bogus time
try:
    v[ 123456 ]
    assert 0, 'expected to fail'
except IndexError:
    pass
# Test clear
v.clear()
assert len(v) == 0
_Validate()
for kf in expected:
    v.SetKeyFrame(kf)
_Validate()
del expected
print('\tPassed')

########################################################################

print('\nTest heterogeneous keyframe types: errors expected')
try:
    v.SetKeyFrame(kf5)
    assert 0, 'expected to fail'
except Tf.ErrorException:
    pass
_Validate()
print('\tPassed')

########################################################################

print('\nTest keyframe slicing')
kf5 = Ts.KeyFrame( kf5_time )
v.SetKeyFrame(kf5)
expected = [kf1, kf2, kf3, kf4, kf5]
assert v[:] == expected
assert v[:kf3.time] == [kf1, kf2]
assert v[kf3.time:] == [kf3, kf4, kf5]
# Test end conditions
assert v[:-1000] == []
assert v[1000:] == []
assert v[-1000:1000] == expected
# Test invalid slice range (start > stop)
assert v[1000:-1000] == []
# Test that stride does not work for slicing
try:
    bad = v[-1000:1000 :1.0]
    assert 0, 'stride should not be allowed'
except ValueError:
    pass
_Validate()
print('\tPassed')

print('\nTest that non-time slice bounds do not work')
try:
    v['foo':'bar']
    assert 0, 'non-time keys should not work for slicing'
except ValueError:
    pass
try:
    v[1:'bar']
    assert 0, 'non-time keys should not work for slicing'
except ValueError:
    pass
_Validate()
print('\tPassed')

print('\nTest that assignment to keyframes[time] does not work')
try:
    v[ 1234 ] = kf1
    assert 0, 'should not have worked'
except AttributeError:
    pass
_Validate()
print('\tPassed')

########################################################################

print('\nTest deleting knots')
assert kf5_time in v.frames
del v[ kf5.time ]
assert kf5_time not in v.frames
_Validate()
print('\tPassed')

print('\nTest deleting keyframe at bogus time')
bogus_time = 1234
assert bogus_time not in v.frames
try:
    del v[ bogus_time ]
    assert 0, 'error expected'
except Tf.ErrorException:
    pass
assert bogus_time not in v.frames
_Validate()
print('\tPassed')

print('\nTest deleting knots by slicing')
# Find some knots to delete; make sure they're valid
keyframes_to_del = [kf2, kf3, kf4]
for kf in keyframes_to_del:
    assert kf.time in v.frames
    assert v[ kf.time ] == kf
assert v[ kf2_time : kf5_time ] == keyframes_to_del
del v[ kf2_time : kf5_time ]
assert v[ kf2_time : kf5_time ] == []
assert kf2_time not in v.frames
assert kf3_time not in v.frames
assert kf4_time not in v.frames
_Validate()
print('\tPassed')

print('\nTest deleting all knots')
assert len(v.frames) > 0
del v[:]
assert len(v.frames) == 0
_Validate()
print('\tPassed')

print('\nTest that deleting knots at bogus frames does not work ' \
    'errors expected')
assert 1234 not in v
try:
    del v[1234]
    assert 0, 'expected failure'
except Tf.ErrorException:
    pass
assert 1234 not in v
print('\tPassed')

print('\nTest that an empty spline returns None for Eval()')
del v[:]
assert len(v.frames) == 0
assert v.Eval(0) is None
assert v.EvalDerivative(0) is None
_Validate()
print('\tPassed')

# Remove these local variables
del kf1
del kf2
del kf3
del kf4
del kf5

########################################################################

print('\nTest changing knot times')
del v[:]
keyframes = [ Ts.KeyFrame(t) for t in range(10) ]
for k in keyframes: 
    v.SetKeyFrame(k)
    _Validate()
assert keyframes == list(v.values())
assert [] != list(v.values())
new_time = 123
assert new_time not in v.keys()
del v[ keyframes[0].time ]
_Validate()
keyframes[0].time = new_time
v.SetKeyFrame( keyframes[0] )
_Validate()
assert new_time in list(v.keys())
assert v[new_time] == keyframes[0]
# We expect that the knot order has changed
assert keyframes != list(v.values())
# Sort our expected knots to reflect new expectations
keyframes.sort(key=lambda x: x.time)
# Check that the new knot order applies
assert keyframes == list(v.values())
del new_time
_Validate()
print('\tPassed')

print('\nTest that changing knot times overwrites existing knots')
keyframes = list(v.values())
kf0_time = keyframes[0].time
kf1_time = keyframes[1].time
oldLen = len(list(v.values()))
assert len(list(v.values())) == oldLen
assert keyframes[0].time != keyframes[1].time
del v[ keyframes[0].time ]
keyframes[0].time = kf1_time
assert keyframes[0].time == kf1_time
_Validate()
v.SetKeyFrame( keyframes[0] )
assert len(list(v.values())) == oldLen - 1
del kf0_time
del kf1_time
del oldLen
_Validate()
print('\tPassed')

print('\nTest frameRange')
del v[:]
assert v.frameRange.isEmpty
v.SetKeyFrame( Ts.KeyFrame(-10, 0.0) )
v.SetKeyFrame( Ts.KeyFrame(123, 0.0) )
assert v.frameRange == Gf.Interval(-10, 123)
_Validate()
print('\tPassed')

print('\nTest extrapolation interface')
x = v.extrapolation
assert x[0] == Ts.ExtrapolationHeld
assert x[1] == Ts.ExtrapolationHeld
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationHeld)
x = v.extrapolation
assert x[0] == Ts.ExtrapolationLinear
assert x[1] == Ts.ExtrapolationHeld
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationLinear)
x = v.extrapolation
assert x[0] == Ts.ExtrapolationHeld
assert x[1] == Ts.ExtrapolationLinear
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
x = v.extrapolation
assert x[0] == Ts.ExtrapolationLinear
assert x[1] == Ts.ExtrapolationLinear
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
x = v.extrapolation
assert x[0] == Ts.ExtrapolationHeld
assert x[1] == Ts.ExtrapolationHeld
print('\tPassed')

print('\nTest that SetKeyFrames() preserves extrapolation')
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationLinear)
x = v.extrapolation
assert x[0] == Ts.ExtrapolationHeld
assert x[1] == Ts.ExtrapolationLinear
oldKeyFrames = list(v.values())
v.SetKeyFrames( [Ts.KeyFrame(0, 0.123)] )
x = v.extrapolation
assert x[0] == Ts.ExtrapolationHeld
assert x[1] == Ts.ExtrapolationLinear
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
v.SetKeyFrames(oldKeyFrames)
print('\tPassed')

########################################################################

print('\nTest Eval() with held knots of various types')
# Loop over various types, with sample keyframe values.
testTypes = {
    'string': ['first', 'second', '', ''],
    'double': [1.0, 2.0, 0.0, 0.0],
    'int':[1,2,0,0],
    'GfVec2d': [ Gf.Vec2d( *list(range(0,0+2)) ),
                 Gf.Vec2d( *list(range(2,2+2)) ), 
                 Gf.Vec2d(), 
                 Gf.Vec2d() ],
    'GfVec2f': [ Gf.Vec2f( *list(range(0,0+2)) ),
                 Gf.Vec2f( *list(range(2,2+2)) ), 
                 Gf.Vec2f(), 
                 Gf.Vec2f() ],
    'GfVec3d': [ Gf.Vec3d( *list(range(0,0+3)) ),
                 Gf.Vec3d( *list(range(3,3+3)) ),
                 Gf.Vec3d(),
                 Gf.Vec3d() ],
    'GfVec3f': [ Gf.Vec3f( *list(range(0,0+3)) ),
                 Gf.Vec3f( *list(range(3,3+3)) ),
                 Gf.Vec3f(),
                 Gf.Vec3f() ],
    'GfVec4d': [ Gf.Vec4d( *list(range(0,0+4)) ),
                 Gf.Vec4d( *list(range(4,4+4)) ),
                 Gf.Vec4d(),
                 Gf.Vec4d() ],
    'GfVec4f': [ Gf.Vec4f( *list(range(0,0+4)) ),
                 Gf.Vec4f( *list(range(4,4+4)) ),
                 Gf.Vec4f(),
                 Gf.Vec4f() ],
    'GfMatrix2d': [ Gf.Matrix2d( *list(range(0,0+4)) ),
                    Gf.Matrix2d( *list(range(4,4+4)) ),
                    Gf.Matrix2d( 0 ),
                    Gf.Matrix2d( 0 ) ],
    'GfMatrix3d': [ Gf.Matrix3d( *list(range(0,0+9)) ),
                    Gf.Matrix3d( *list(range(9,9+9)) ),
                    Gf.Matrix3d( 0 ),
                    Gf.Matrix3d( 0 ) ],
    'GfMatrix4d': [ Gf.Matrix4d( *list(range(0,0+16)) ),
                    Gf.Matrix4d( *list(range(16,16+16)) ),
                    Gf.Matrix4d( 0 ),
                    Gf.Matrix4d( 0 ) ]
    }
for testType in testTypes:
    v0, v1, d0, d1 = testTypes[testType]
    del v[:]
    t0 = 0
    t1 = 10
    kf0 = Ts.KeyFrame(t0, v0, Ts.KnotHeld)
    kf1 = Ts.KeyFrame(t1, v1, Ts.KnotHeld)
    if not v.CanSetKeyFrame(kf0) or not v.CanSetKeyFrame(kf1):
        continue
    v.SetKeyFrame(kf0)
    _Validate()
    v.SetKeyFrame(kf1)
    _Validate()
    assert v.typeName == testType
    assert v.Eval(t0-1) == v0
    assert v.Eval(t0-0.5) == v0
    assert v.Eval(t0+0) == v0
    assert v.Eval(t0+0.5) == v0
    assert v.Eval(t0+1) == v0
    assert v.Eval(t1-1) == v0
    assert v.Eval(t1-0.5) == v0
    assert v.Eval(t1+0) == v1
    assert v.Eval(t1+0.5) == v1
    assert v.Eval(t1+1) == v1

    assert v.EvalDerivative(t0-1) == d0
    assert v.EvalDerivative(t0-0.5) == d0
    assert v.EvalDerivative(t0+0) == d0
    assert v.EvalDerivative(t0+0.5) == d0
    assert v.EvalDerivative(t0+1) == d0
    assert v.EvalDerivative(t1-1) == d0
    assert v.EvalDerivative(t1-0.5) == d0
    assert v.EvalDerivative(t1+0) == d1
    assert v.EvalDerivative(t1+0.5) == d1
    assert v.EvalDerivative(t1+1) == d1
print('\tPassed')

########################################################################

del v[:]

print('\nTest float Eval() with linear knots')
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotLinear ) )
_Validate()
v.SetKeyFrame( Ts.KeyFrame( 10, value = 20.0, knotType = Ts.KnotLinear ) )
_Validate()
assert v.Eval(-5) == 0
assert v.Eval(2.5) == 5
assert v.Eval( 5) == 10
assert v.Eval(7.5) == 15
assert v.Eval(15) == 20
assert v.EvalDerivative(-5) == 0
assert v.EvalDerivative(2.5) == 2
assert v.EvalDerivative(5) == 2
assert v.EvalDerivative(7.5) == 2
assert v.EvalDerivative(15) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-5) == -10
assert v.Eval(2.5) == 5
assert v.Eval( 5) == 10
assert v.Eval(7.5) == 15
assert v.Eval(15) == 30
assert v.EvalDerivative(-5) == 2
assert v.EvalDerivative(2.5) == 2
assert v.EvalDerivative(5) == 2
assert v.EvalDerivative(7.5) == 2
assert v.EvalDerivative(15) == 2
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print('\tPassed')

print('\nTest float Range() with linear knots')
assert v.Range(-1, 11) == (0, 20)
assert v.Range(-1, -1) == (0, 0)
assert v.Range(-1, 0) == (0, 0)
assert v.Range(0, 10) == (0, 20)
assert v.Range(2.5, 7.5) == (5, 15)
assert v.Range(5, 5) == (10, 10)
assert v.Range(10, 11) == (20, 20)
assert v.Range(11, 11) == (20, 20)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Range(-1, 11) == (0, 20)
assert v.Range(-1, -1) == (0, 0)
assert v.Range(-1, 0) == (0, 0)
assert v.Range(0, 10) == (0, 20)
assert v.Range(2.5, 7.5) == (5, 15)
assert v.Range(5, 5) == (10, 10)
assert v.Range(10, 11) == (20, 20)
assert v.Range(11, 11) == (20, 20)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest extrapolation with one linear knot')
del v[10]
assert v.Eval(-5) == 0
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-5) == 0
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print('\tPassed')

del v[:]

print('\nTest float Eval() with dual-value linear knots')
kf1 = Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotLinear )
kf2 = Ts.KeyFrame( 10, value = 0.0, knotType = Ts.KnotLinear )
assert not kf1.isDualValued
assert not kf2.isDualValued
kf1.value = (10, 0)
kf2.value = (10, 0)
assert kf1.isDualValued
assert kf2.isDualValued
v.SetKeyFrames( [kf1, kf2] )
assert v.Eval(-1) == 10
assert v.Eval( 0) ==  0
assert v.Eval( 1) ==  1
assert v.Eval( 9) ==  9
assert Gf.IsClose(v.Eval( 9.99), 9.99, EPSILON)
assert v.Eval(10) ==  0
assert v.Eval(11) ==  0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-1) == 10
assert v.Eval( 0) ==  0
assert v.Eval( 1) ==  1
assert v.Eval( 9) ==  9
assert Gf.IsClose(v.Eval( 9.99), 9.99, EPSILON)
assert v.Eval(10) ==  0
assert v.Eval(11) ==  0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest float Range() with dual-value linear knots')
assert v.Range(-1, 11) == (0, 10)
assert v.Range(-1, -1) == (10, 10)
assert v.Range(-1, 0) == (0, 10)
assert v.Range(0, 10) == (0, 10)
assert v.Range(2.5, 7.5) == (2.5, 7.5)
assert v.Range(5, 5) == (5, 5)
assert v.Range(10, 11) == (0, 0)
assert v.Range(11, 11) == (0, 0)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Range(-1, 11) == (0, 10)
assert v.Range(-1, -1) == (10, 10)
assert v.Range(-1, 0) == (0, 10)
assert v.Range(0, 10) == (0, 10)
assert v.Range(2.5, 7.5) == (2.5, 7.5)
assert v.Range(5, 5) == (5, 5)
assert v.Range(10, 11) == (0, 0)
assert v.Range(11, 11) == (0, 0)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest extrapolation with one dual-value linear knot')
del v[10]
assert v.Eval(-5) == 10
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-5) == 10
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print('\tPassed')

del v[:]

print('\nTest float Eval() with Bezier knots w/ zero-length handles')
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0, leftSlope = 0, rightLen = 0, rightSlope = 0 ) )
v.SetKeyFrame( Ts.KeyFrame( 10, value = 10.0, knotType = Ts.KnotBezier,
    leftLen = 0, leftSlope = 0, rightLen = 0, rightSlope = 0 ) )
assert Gf.IsClose(v.Eval(-1), 0, EPSILON)
assert Gf.IsClose(v.Eval(0), 0, EPSILON)
assert Gf.IsClose(v.Eval(1), 1, EPSILON)
assert Gf.IsClose(v.Eval(9), 9, EPSILON)
assert Gf.IsClose(v.Eval(10), 10, EPSILON)
assert Gf.IsClose(v.Eval(11), 10, EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert Gf.IsClose(v.Eval(-1), 0, EPSILON)
assert Gf.IsClose(v.Eval(0), 0, EPSILON)
assert Gf.IsClose(v.Eval(1), 1, EPSILON)
assert Gf.IsClose(v.Eval(9), 9, EPSILON)
assert Gf.IsClose(v.Eval(10), 10, EPSILON)
assert Gf.IsClose(v.Eval(11), 10, EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest float Range() with Bezier knots w/ zero-length handles')
assert Gf.IsClose(v.Range(-1, 11), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(-1, -1), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 10), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1, 9), EPSILON)
assert Gf.IsClose(v.Range(5, 5), (5, 5), EPSILON)
assert Gf.IsClose(v.Range(10, 11), (10, 10), EPSILON)
assert Gf.IsClose(v.Range(11, 11), (10, 10), EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert Gf.IsClose(v.Range(-1, 11), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(-1, -1), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 10), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1, 9), EPSILON)
assert Gf.IsClose(v.Range(5, 5), (5, 5), EPSILON)
assert Gf.IsClose(v.Range(10, 11), (10, 10), EPSILON)
assert Gf.IsClose(v.Range(11, 11), (10, 10), EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest extrapolation with one Bezier knot w/ zero-length handles')
del v[10]
assert v.Eval(-5) == 0
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-5) == 0
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print('\tPassed')

del v[:]

print('\nTest float Eval() with Bezier knots')
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 0,
    rightLen = 0.5, rightSlope = 0 ) )
v.SetKeyFrame( Ts.KeyFrame( 5, value = 10.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 0,
    rightLen = 0.5, rightSlope = 0 ) )
assert Gf.IsClose(v.Eval(-1), 0, EPSILON)
assert Gf.IsClose(v.Eval(0), 0, EPSILON)
assert Gf.IsClose(v.Eval(1), 1.7249, COARSE_EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert Gf.IsClose(v.Eval(-1), 0, EPSILON)
assert Gf.IsClose(v.Eval(0), 0, EPSILON)
assert Gf.IsClose(v.Eval(1), 1.7249, COARSE_EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

del v[:]

print('\nTest float Eval() with Bezier knots (2)')
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 1,
    rightLen = 0.5, rightSlope = -1 ) )
v.SetKeyFrame( Ts.KeyFrame( 5, value = 10.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 1,
    rightLen = 0.5, rightSlope = 1 ) )
v.SetKeyFrame( Ts.KeyFrame( 10, value = 10.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = -1,
    rightLen = 0.5, rightSlope = 1 ) )
v.SetKeyFrame( Ts.KeyFrame( 15, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 1,
    rightLen = 0.5, rightSlope = 1 ) )
assert v.Eval(-1) == 0
assert v.Eval(0) == 0
assert Gf.IsClose(v.Eval(1), 1.4333, COARSE_EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-1) == -1
assert v.Eval(0) == 0
assert Gf.IsClose(v.Eval(1), 1.4333, COARSE_EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest float Range() with Bezier knots')
assert Gf.IsClose(v.Range(-1, 16), (-0.018137, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(-1, -1), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 15), (-0.018137, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1.43337, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 1), (1.43337, 1.43337), COARSE_EPSILON)
assert Gf.IsClose(v.Range(15, 16), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(16, 16), (0, 0), EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert Gf.IsClose(v.Range(-1, 16), (-0.018137, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(-1, -1), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 15), (-0.018137, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1.43337, 10.375), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 1), (1.43337, 1.43337), COARSE_EPSILON)
assert Gf.IsClose(v.Range(15, 16), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(16, 16), (0, 0), EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest extrapolation with one Bezier knot')
del v[5]
del v[10]
del v[15]
assert v.Eval(-5) == 0
assert v.Eval( 5) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Eval(-5) == -5
assert v.Eval( 5) == -5
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print('\tPassed')

print('\nTest float Eval() with Bezier knots (3)')
del v[:]
# For code coverage, we construct a case that will exercise different
# paths of the black box evaluation code.
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0.0, leftSlope = 0, rightLen = 0.0, rightSlope = 0 ) )
v.SetKeyFrame( Ts.KeyFrame( 9, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 6.0, leftSlope = 0, rightLen = 0.0, rightSlope = 0 ) )
v.Eval(0)
v.Eval(0.5)
# And another case...
del v[:]
v.SetKeyFrame( Ts.KeyFrame( 1, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 0.0, leftSlope = 0, rightLen = 0.0, rightSlope = 0 ) )
v.SetKeyFrame( Ts.KeyFrame( 7, value = 0.0, knotType = Ts.KnotBezier,
    leftLen = 4.0, leftSlope = 0, rightLen = 0.0, rightSlope = 0 ) )
v.Eval(0)
v.Eval(0.5)
_Validate()
print('\tPassed')

del v[:]

v0 = Gf.Vec3d( 1, 1, 1 )
v1 = Gf.Vec3d( 11, 21, 31 )
kf0 = Ts.KeyFrame(0, value = v0, knotType = Ts.KnotLinear)
kf1 = Ts.KeyFrame(10, value = v1, knotType = Ts.KnotLinear)
if v.CanSetKeyFrame(kf0):
    print('\nTest Gf.Vec3d Eval() with linear knots')
    v.SetKeyFrame(kf0)
    v.SetKeyFrame(kf1)
    def blend(a, b, u):
        return a*(1-u) + b*u
    assert v.Eval(-1) == v0
    assert v.Eval(0) == v0
    assert Gf.IsClose(v.Eval(1), blend( v0, v1, 0.1 ), EPSILON)
    assert Gf.IsClose(v.Eval(9), blend( v0, v1, 0.9 ), EPSILON)
    assert v.Eval(10) == v1
    assert v.Eval(11) == v1
    v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert Gf.IsClose(v.Eval(-1), blend( v0, v1, -0.1 ), EPSILON)
    assert v.Eval(0) == v0
    assert Gf.IsClose(v.Eval(1), blend( v0, v1, 0.1 ), EPSILON)
    assert Gf.IsClose(v.Eval(9), blend( v0, v1, 0.9 ), EPSILON)
    assert v.Eval(10) == v1
    assert Gf.IsClose(v.Eval(11), blend( v0, v1, 1.1 ), EPSILON)
    v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    _Validate()
    del blend
    print('\tPassed')

print('\nTest Eval() with multiple keyframes given')
v.clear()
kf1 = Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotLinear )
kf2 = Ts.KeyFrame( 10, value = 0.0, knotType = Ts.KnotLinear )
v.SetKeyFrame(kf1)
v.SetKeyFrame(kf2)
keyframes = list(v[:])
assert len(keyframes) > 1
times = [kf.time for kf in keyframes]
expectedValues = [ v.Eval(t) for t in times ]
assert expectedValues == list( v.Eval(times) )
_Validate()
print('\tPassed')

print('\nTest float Range() with mixed knot types')
del v[:]
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotLinear ) )
v.SetKeyFrame( Ts.KeyFrame( 10, value = 10.0, knotType = Ts.KnotBezier,
    leftLen = 0.5, leftSlope = 0,
    rightLen = 0.5, rightSlope = 0 ) )
assert Gf.IsClose(v.Range(-1, 11), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 10), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1.01184, 9.19740), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 1), (1.01184, 1.01184), COARSE_EPSILON)
assert Gf.IsClose(v.Range(10, 11), (10, 10), EPSILON)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert Gf.IsClose(v.Range(-1, 11), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(-1, 0), (0, 0), EPSILON)
assert Gf.IsClose(v.Range(0, 10), (0, 10), EPSILON)
assert Gf.IsClose(v.Range(1, 9), (1.01184, 9.19740), COARSE_EPSILON)
assert Gf.IsClose(v.Range(1, 1), (1.01184, 1.01184), COARSE_EPSILON)
assert Gf.IsClose(v.Range(10, 11), (10, 10), EPSILON)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest float Range() with empty TsValue')
del v[:]
assert v.Range(0, 10) == (None, None)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
assert v.Range(0, 10) == (None, None)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
_Validate()
print('\tPassed')

print('\nTest float Range() with bad time domain:  errors expected')
try:
    v.Range(10, 0)
    assert False, "should have failed"
except Tf.ErrorException:
    pass
_Validate()
print('\tPassed')

del v[:]

# Note that Range() cannot interpolate strings
kf0 = Ts.KeyFrame( 0, "foo", Ts.KnotHeld )
kf1 = Ts.KeyFrame( 10, "bar", Ts.KnotHeld )
if v.CanSetKeyFrame(kf0):
    print('\nTest string Range()')
    v.SetKeyFrame(kf0)
    v.SetKeyFrame(kf1)
    assert v.Range(0, 10) == (None, None)
    v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert v.Range(0, 10) == (None, None)
    v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    _Validate()
    print('\tPassed')

########################################################################

del v[:]

v0 = Vt.DoubleArray(3)
v0[:] = (1, 1, 1)
v1 = Vt.DoubleArray(3)
v1[:] = (10, 20, 30)
kf0 = Ts.KeyFrame(0, value = v0, knotType = Ts.KnotBezier)
kf1 = Ts.KeyFrame(10, value = v1, knotType = Ts.KnotBezier)
if v.CanSetKeyFrame(kf0):
    print('\nTest Eval() with VtArray<double> and bezflat knots')
    v.SetKeyFrame(kf0)
    v.SetKeyFrame(kf1)
    assert v.Eval(-1) == v0
    assert v.Eval(0) == v0
    assert list( v.Eval(5) ) == [5.5, 10.5, 15.5]
    assert v.Eval(10) == v1
    assert v.Eval(11) == v1
    v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert Gf.IsClose(list( v.Eval(-1) ), [0.1, -0.9, -1.9], EPSILON)
    assert v.Eval(0) == v0
    assert list( v.Eval(5) ) == [5.5, 10.5, 15.5]
    assert v.Eval(10) == v1
    assert list( v.Eval(11) ) == [10.9, 21.9, 32.9]
    v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    v2 = Vt.DoubleArray(3)
    v2[:] = (0,0,0)
    v.SetKeyFrame( Ts.KeyFrame(0, value = v2, knotType = Ts.KnotBezier))
    assert v.Eval(-1) == v2
    assert v.Eval(0) == v2
    assert list( v.Eval(5) ) == [5, 10, 15]
    assert v.Eval(10) == v1
    assert v.Eval(11) == v1
    v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert list( v.Eval(-1) ) == [-1, -2, -3]
    assert v.Eval(0) == v2
    assert list( v.Eval(5) ) == [5, 10, 15]
    assert v.Eval(10) == v1
    assert list( v.Eval(11) ) == [11, 22, 33]
    v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    _Validate()
    print('\tPassed')

########################################################################

del v[:]

print('\nTest that deleting knots properly affects evaluation')
v.SetKeyFrame( Ts.KeyFrame( 0, value = 0.0, knotType = Ts.KnotLinear ) )
v.SetKeyFrame( Ts.KeyFrame( 10, value = 20.0, knotType = Ts.KnotLinear ) )
# Displace the middle knot
midKnot = v.ClosestKeyFrame(5)
midKnot.value = 100
# Take a sample that depends on the middle knot value
test_time = 2.5
test_val = v.Eval(test_time)
assert v.Eval(test_time) == test_val
del v[midKnot.time]
assert v.Eval(test_time) != test_val
del test_val
del test_time
del midKnot
_Validate()
print('\tPassed')

########################################################################

del v[:]

print('\nTest "closest keyframe" style access on empty value')
for t in [-1, 0, 1]:
    assert not v.ClosestKeyFrame(t)
    assert not v.ClosestKeyFrameBefore(t)
    assert not v.ClosestKeyFrameAfter(t)
_Validate()
print('\tPassed')

print('\nTest "closest keyframe" style access')
kf1 = Ts.KeyFrame( -1 )
kf2 = Ts.KeyFrame( 0 )
kf3 = Ts.KeyFrame( 1 )
keyframes = [kf1, kf2, kf3]
for kf in keyframes:
    v.SetKeyFrame(kf)

for kf in keyframes:
    assert v.ClosestKeyFrame( kf.time - 0.1 ) == kf
    assert v.ClosestKeyFrame( kf.time + 0.0 ) == kf
    assert v.ClosestKeyFrame( kf.time + 0.1 ) == kf

# Test edge cases
assert v.ClosestKeyFrameBefore( kf1.time - 0.1 ) is None
assert v.ClosestKeyFrameBefore( kf2.time - 0.1 ) == kf1
assert v.ClosestKeyFrameBefore( kf3.time - 0.1 ) == kf2
assert v.ClosestKeyFrameBefore( kf1.time ) is None
assert v.ClosestKeyFrameBefore( kf2.time ) == kf1
assert v.ClosestKeyFrameBefore( kf3.time ) == kf2
assert v.ClosestKeyFrameBefore( kf1.time + 0.1 ) == kf1
assert v.ClosestKeyFrameBefore( kf2.time + 0.1 ) == kf2
assert v.ClosestKeyFrameBefore( kf3.time + 0.1 ) == kf3

# Test edge cases
assert v.ClosestKeyFrameAfter( kf1.time - 0.1 ) == kf1
assert v.ClosestKeyFrameAfter( kf2.time - 0.1 ) == kf2
assert v.ClosestKeyFrameAfter( kf3.time - 0.1 ) == kf3
assert v.ClosestKeyFrameAfter( kf1.time ) == kf2
assert v.ClosestKeyFrameAfter( kf2.time ) == kf3
assert v.ClosestKeyFrameAfter( kf3.time ) is None
assert v.ClosestKeyFrameAfter( kf1.time + 0.1 ) == kf2
assert v.ClosestKeyFrameAfter( kf2.time + 0.1 ) == kf3
assert v.ClosestKeyFrameAfter( kf3.time + 0.1 ) is None

_Validate()
print('\tPassed')

########################################################################
# Test breakdown

leftKeyFrames = {
    Ts.KnotHeld:       Ts.KeyFrame(0, value = 0.0, knotType = Ts.KnotHeld),
    Ts.KnotLinear:     Ts.KeyFrame(0, value = 0.0, knotType = Ts.KnotLinear),
    Ts.KnotBezier: Ts.KeyFrame(0, value = 0.0,
                                   knotType = Ts.KnotBezier,
                                   leftLen = 2.0, leftSlope = 0.0,
                                   rightLen = 2.0, rightSlope = 0.0 )
    }
rightKeyFrames = {
    Ts.KnotHeld:       Ts.KeyFrame(12, value = 12.0, knotType = Ts.KnotHeld),
    Ts.KnotLinear:     Ts.KeyFrame(12, value = 12.0, knotType = Ts.KnotLinear),
    Ts.KnotBezier: Ts.KeyFrame(12, value = 12.0,
                                   knotType = Ts.KnotBezier,
                                   leftLen = 2.0, leftSlope = 0.0,
                                   rightLen = 2.0, rightSlope = 0.0 )
    }
breakdownFlatKeyFrames = {
    Ts.KnotHeld:       Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotHeld),
    Ts.KnotLinear:     Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotLinear),
    Ts.KnotBezier: Ts.KeyFrame(6, value = 0.0,
                                   knotType = Ts.KnotBezier,
                                   leftLen = 1.0, leftSlope = 0.0,
                                   rightLen = 1.0, rightSlope = 0.0 )
    }
breakdownNonFlatKeyFrames = {
    Ts.KnotHeld: {
        Ts.KnotHeld:       Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotHeld),
        Ts.KnotLinear:     Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotLinear),
        Ts.KnotBezier: Ts.KeyFrame(6, value = 0.0,
                                       knotType = Ts.KnotBezier,
                                       leftLen = 2.0,
                                       leftSlope = 0.0,
                                       rightLen = 2.0,
                                       rightSlope = 0.0 )
        },
    Ts.KnotLinear: {
        Ts.KnotHeld:       Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotHeld),
        Ts.KnotLinear:     Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotLinear),
        Ts.KnotBezier: Ts.KeyFrame(6, value = 0.0,
                                       knotType = Ts.KnotBezier,
                                       leftLen = 2.0,
                                       leftSlope = 1.0,
                                       rightLen = 2.0,
                                       rightSlope = 1.0 )
        },
    Ts.KnotBezier: {
        Ts.KnotHeld:       Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotHeld),
        Ts.KnotLinear:     Ts.KeyFrame(6, value = 0.0, knotType = Ts.KnotLinear),
        Ts.KnotBezier: Ts.KeyFrame(6, value = 0.0,
                                       knotType = Ts.KnotBezier,
                                       leftLen = 2.5,
                                       leftSlope = 1.2,
                                       rightLen = 2.5,
                                       rightSlope = 1.2 )
        }
    }

for knotType in leftKeyFrames:
    for breakdownType in breakdownFlatKeyFrames:
        print('\nTest float Breakdown() with %s knots w/ flat %s knot' % \
                    (knotType, breakdownType))
        del v[:]
        v.SetKeyFrame( leftKeyFrames[knotType] )
        v.SetKeyFrame( rightKeyFrames[knotType] )
        x = v.Eval(6);
        v.Breakdown(6, breakdownType, True, 1.0)

        # Verify values
        assert Gf.IsClose(v.Eval(0), 0, EPSILON)
        assert Gf.IsClose(v.Eval(6), x, EPSILON)
        assert Gf.IsClose(v.Eval(12), 12, EPSILON)
        # Verify key frame
        breakdownFlatKeyFrames[breakdownType].value = x
        assert(v[6] == breakdownFlatKeyFrames[breakdownType])

        _Validate()
        print('\tPassed')

    for breakdownType in breakdownNonFlatKeyFrames:
        print('\nTest float Breakdown() with %s knots w/ non-flat %s knot' % \
                    (knotType, breakdownType))
        del v[:]
        v.SetKeyFrame( leftKeyFrames[knotType] )
        v.SetKeyFrame( rightKeyFrames[knotType] )
        x = v.Eval(6);
        v.Breakdown(6, breakdownType, False, 1.0)

        # Verify values
        assert Gf.IsClose(v.Eval(0), 0, EPSILON)
        assert Gf.IsClose(v.Eval(6), x, EPSILON)
        assert Gf.IsClose(v.Eval(12), 12, EPSILON)
        # Verify key frame
        breakdownNonFlatKeyFrames[knotType][breakdownType].value = x
        assert(v[6] == breakdownNonFlatKeyFrames[knotType][breakdownType])

        _Validate()
        print('\tPassed')

# Test vectorized breakdown
types = [Ts.KnotHeld, Ts.KnotLinear, Ts.KnotBezier]
times = [0.0, 1.0, 2.0, 3.0]
tangentLength = 100.0

print('\nTest vectorized Breakdown()')
for type in types:
    # Reset the spline with some curvature to test
    del v[:]
    k1 = v.Breakdown(-1.0, Ts.KnotBezier, True, tangentLength, 0.0)
    k2 = v.Breakdown(4.0, Ts.KnotBezier, True, tangentLength, 200.0)

    # Save the initial values
    values = {}
    for t in times:
        values[t] = v.Eval(t)

    # Vectorized breakdown. Note that the following loop would produce a
    # different result because each iteration affects the next:
    #
    #     for t in times:
    #         b.Breakdown(t, type, True, tangentLength)
    #
    breakdownTimes = times + [k1.time, k2.time]
    result = v.Breakdown(breakdownTimes, type, True, tangentLength)

    print(list(result.keys()))
    assert set(result.keys()) == set(breakdownTimes)
    assert result[k1.time] == k1
    assert result[k2.time] == k2

    for t in times:
        assert t in v
        assert v[t].knotType == type
        assert v[t].value == values[t]

        if (type == Ts.KnotBezier):
            assert v[t].leftSlope == 0.0
            assert v[t].rightSlope == 0.0
            assert v[t].leftLen == tangentLength
            assert v[t].rightLen == tangentLength

# Test breakdown with multiple values and knot types
print('\nTest Breakdown() with multiple values and knot types')
types = [Ts.KnotHeld, Ts.KnotLinear, Ts.KnotBezier]
times = [0.0, 1.0, 2.0]
values = [2.0, -2.0, 8.0]
tangentLength = 10.0

# Reset the spline with some curvature to test
del v[:]
k1 = v.Breakdown(-1.0, Ts.KnotBezier, True, tangentLength, 0.0)
k2 = v.Breakdown(4.0, Ts.KnotBezier, True, tangentLength, 200.0)

breakdownTimes = times + [k1.time, k2.time]
breakdownTypes = types + [k1.knotType, k2.knotType]
breakdownValues = values + [k1.value, k2.value]
result = v.Breakdown(breakdownTimes, breakdownTypes, False, tangentLength, \
                     breakdownValues)

print(list(result.keys()))
assert set(result.keys()) == set(breakdownTimes)
assert result[k1.time] == k1
assert result[k2.time] == k2

for i, t in enumerate(times):
    assert t in v
    assert v[t].knotType == types[i]
    assert v[t].value == values[i]

########################################################################
# Test Sample()

def _IsAbsClose(a, b, epsilon):
    return abs(a-b) < epsilon 

def _CheckSamples(val, samples, startTime, endTime, tolerance):
    for i,s in enumerate(samples):
        assert not s.isBlur
        assert s.leftTime <= s.rightTime
        if i != 0:
            assert samples[i - 1].rightTime <= samples[i].leftTime
        if s.leftTime >= startTime:
            assert _IsAbsClose(s.leftValue,
                               val.Eval(s.leftTime, Ts.Right),
                               tolerance)
        if s.rightTime <= endTime:
            assert _IsAbsClose(s.rightValue,
                               val.Eval(s.rightTime, Ts.Left),
                               tolerance)
    if samples:
        assert samples[0].leftTime <= startTime
        assert samples[-1].rightTime >= endTime

# Sample to within this error tolerance
tolerance = 1.0e-3

# Maximum allowed error is not tolerance, it's much larger.  This
# is because Eval() samples differently between frames than at
# frames and will yield slightly incorrect results but avoid
# problems with large derivatives.  Sample() does not do that.
maxError  = 0.15

print("\nTest Sample() with bad time domain:  errors expected\n")
try:
    samples = v.Sample(11, -1, 1.0, 1.0, tolerance)
    assert False, 'exception expected'
except Tf.ErrorException:
    pass
print("\tpassed\n")

print("\nTest Sample() with no knots\n")
v.clear()
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
assert len(samples) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
assert len(samples) == 0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

print("\nTest Sample() with empty time domain\n")
v.SetKeyFrame( Ts.KeyFrame(0, 0.0, Ts.KnotHeld) )
v.SetKeyFrame( Ts.KeyFrame(10, 10.0, Ts.KnotHeld) )
samples = v.Sample(3, 3, 1.0, 1.0, tolerance)
assert len(samples) == 0
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(3, 3, 1.0, 1.0, tolerance)
assert len(samples) == 0
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

print("\nTest double Sample() with held knots\n")
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 11, maxError)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 11, maxError)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

v.clear()
kf0 = Ts.KeyFrame(0, "foo", Ts.KnotHeld)
if v.CanSetKeyFrame(kf0):
    print("\nTest string Sample() with held knots\n")
    v.clear()
    v.SetKeyFrame( Ts.KeyFrame(0, "foo", Ts.KnotHeld) )
    v.SetKeyFrame( Ts.KeyFrame(10, "bar", Ts.KnotHeld) )
    samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
    assert len(samples) == 3
    assert samples[0].leftValue == "foo"
    assert samples[1].leftValue == "foo"
    assert samples[2].leftValue == "bar"
    assert samples[0].leftValue == samples[0].rightValue
    assert samples[1].leftValue == samples[1].rightValue
    assert samples[2].leftValue == samples[2].rightValue
    v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
    assert len(samples) == 3
    assert samples[0].leftValue == "foo"
    assert samples[1].leftValue == "foo"
    assert samples[2].leftValue == "bar"
    assert samples[0].leftValue == samples[0].rightValue
    assert samples[1].leftValue == samples[1].rightValue
    assert samples[2].leftValue == samples[2].rightValue
    v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    print("\tpassed\n")

print("\nTest Sample() with linear knots\n")
v.clear()
v.SetKeyFrame( Ts.KeyFrame(0, 0.0, Ts.KnotLinear) )
v.SetKeyFrame( Ts.KeyFrame(10, 10.0, Ts.KnotLinear) )
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 11, maxError)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(-1, 11, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 11, maxError)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

print("\nTest Sample() with Bezier knots\n")
v.clear()
v.SetKeyFrame( Ts.KeyFrame(0, 0.0, Ts.KnotBezier,
                               1.0, 0.5, 1.0, 0.5) )
v.SetKeyFrame( Ts.KeyFrame(5, 10.0, Ts.KnotBezier,
                               1.0, 0.5, 1.0, 0.5) )
v.SetKeyFrame( Ts.KeyFrame(10, 10.0, Ts.KnotBezier,
                               -1.0, 0.5, 1.0, 0.5) )
v.SetKeyFrame( Ts.KeyFrame(15, 0.0, Ts.KnotBezier,
                               1.0, 0.5, 1.0, 0.5) )
samples = v.Sample(-1, 16, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 16, maxError)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(-1, 16, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 16, maxError)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

print("\nTest Sample() with mixed knot types\n")
v.clear()
v.SetKeyFrame( Ts.KeyFrame(0, 0.0, Ts.KnotLinear) )
v.SetKeyFrame( Ts.KeyFrame(5, 5.0, Ts.KnotBezier,
                               0.0, 0.5, 0.0, 0.5) )
v.SetKeyFrame( Ts.KeyFrame(10, 10.0, Ts.KnotLinear) )
samples = v.Sample(-1, 16, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 16, maxError)
v.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
samples = v.Sample(-1, 16, 1.0, 1.0, tolerance)
_CheckSamples(v, samples, -1, 16, maxError)
v.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
print("\tpassed\n")

############################################################################

def TestHeldEval():
    print("\nTest held evaluation:")

    spline = v
    spline.clear()
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationLinear)

    spline.SetKeyFrame(Ts.KeyFrame(time = 0.0, value = 0.0,
                                     knotType = Ts.KnotBezier,
                                     leftSlope = -1.0, rightSlope = -1.0,
                                     leftLen = 1.0, rightLen = 1.0))

    spline.SetKeyFrame(Ts.KeyFrame(time = 5.0, value = 1.0,
                                     knotType = Ts.KnotBezier,
                                     leftSlope = 0.0, rightSlope = 0.0,
                                     leftLen = 1.0, rightLen = 1.0))

    spline.SetKeyFrame(Ts.KeyFrame(time = 10.0, value = 0.0,
                                     knotType = Ts.KnotBezier,
                                     leftSlope = 1.0, rightSlope = 1.0,
                                     leftLen = 1.0, rightLen = 1.0))


    # Check our math for normal bezier interpolation.
    assert spline.Eval(-2.5) == 2.5
    assert spline.Eval(2.5) == 0.125
    assert spline.Eval(7.5) == 0.125
    assert spline.Eval(12.5) == 2.5

    # Check with held evaluation. Extrapolation and interpolation should both
    # behave as if "held".
    assert spline.EvalHeld(-2.5) == 0.0
    assert spline.EvalHeld(2.5) == 0.0
    assert spline.EvalHeld(7.5) == 1.0
    assert spline.EvalHeld(12.5) == 0.0

    # Check held evaluation with side argument.
    assert spline.EvalHeld(5.0) == 1.0
    assert spline.EvalHeld(5.0, side=Ts.Right) == 1.0
    assert spline.EvalHeld(5.0, side=Ts.Left) == 0.0

    assert spline.EvalHeld(10.0) == 0.0
    assert spline.EvalHeld(10.0, side=Ts.Right) == 0.0
    assert spline.EvalHeld(10.0, side=Ts.Left) == 1.0

    # Check held evaluation with dual-valued knot. Held evaluation on the
    # left side of a dual-valued knot should actually give us the value of
    # the previous keyframe, not the left value.
    kf = spline[5.0]
    kf.value = (-1.0, 1.0)
    spline.SetKeyFrame(kf)

    assert spline.EvalHeld(5.0) == 1.0
    assert spline.EvalHeld(5.0, side=Ts.Right) == 1.0
    assert spline.EvalHeld(5.0, side=Ts.Left) == 0.0

    print("\tpassed")

TestHeldEval()

############################################################################

def TestLooping():
    print("\nTest looping splines:")
    # value offset used below
    offset = 2.3

    # Test LoopParams
    params = Ts.LoopParams(True, 10, 20, 30, 40, offset)
    assert params.looping == True
    assert params.GetMasterInterval() == Gf.Interval(10, 30, True, False)
    assert params.GetLoopedInterval() == Gf.Interval(-20, 70, True, False)
    assert params.valueOffset == offset
    # Equality
    assert params == Ts.LoopParams(True, 10, 20, 30, 40, offset)
    spline = v
    spline.extrapolation = (Ts.ExtrapolationHeld,
                            Ts.ExtrapolationHeld)
    spline.clear()
    # Add some knots in the master region to be unrolled
    v.SetKeyFrame( Ts.KeyFrame( 10, value = 10.0, knotType = Ts.KnotLinear ) )
    v.SetKeyFrame( Ts.KeyFrame( 20, value = 20.0, knotType = Ts.KnotLinear ) )
    # Add a knot in the region to be unrolled; it will be "hidden"
    v.SetKeyFrame( Ts.KeyFrame( 35, value = 100.0, knotType = Ts.KnotLinear ) )
    # Loop it, eval at various places, should be a triangle wave with 1.5
    # periods of prepeat and 2 periods of repeat, with held extrapolation
    # beyond
    v.loopParams = params
    # Check the keframe count
    assert len(v.frames) == 9
    # Check all the keyframe values
    assert v.Eval(10) == 10
    assert v.Eval(20) == 20
    # repeat region from 30 through 70
    assert v.Eval(30) == 10 + offset
    assert v.Eval(40) == 20 + offset
    assert v.Eval(50) == 10 + 2 * offset
    assert v.Eval(60) == 20 + 2 * offset
    # held from here on out
    assert v.Eval(61) == 20 + 2 * offset
    # prepeat region from -20 through 0
    assert v.Eval(0)   == 20 - offset
    assert v.Eval(-10) == 10 - offset
    assert v.Eval(-20) == 20 - 2 * offset
    # held from here on out
    assert v.Eval(-21) == 20 - 2 * offset
    # eval between knots in the unrolled region, this is where there's
    # a knot "hidden" by the unrolling
    assert v.Eval(35) == 15 + offset

    # add a key in the master region; should be written
    v.SetKeyFrame( Ts.KeyFrame( 15, value = 50.0, 
        knotType = Ts.KnotLinear ) )
    assert v.Eval(15) == 50
    # because of unrolling, really added 4 frames
    assert len(v.frames) == 13

    # add keys before and after the pre and repeat; should be written
    v.SetKeyFrame( Ts.KeyFrame( -200, value = -200.0, 
        knotType = Ts.KnotLinear ) )
    v.SetKeyFrame( Ts.KeyFrame( 200, value = 200.0, 
        knotType = Ts.KnotLinear ) )
    assert len(v.frames) == 15
    assert v.Eval(-200) == -200
    assert v.Eval(200) == 200

    # add a key in the unrolled region; should be ignored
    oldVal = v.Eval(35)
    v.SetKeyFrame( Ts.KeyFrame( 35, value = 50.0, 
        knotType = Ts.KnotLinear ) )
    assert v.Eval(35) == oldVal
    assert len(v.frames) == 15

    # remove a key in the master region
    del v[ 15.0 ]
    assert v.Eval(15) != 50
    # because of unrolling, really removed 4 frames
    assert len(v.frames) == 11

    # turn off looping and see if the "hidden" knot is still there, and
    # if the one at 15 got removed; should result in a count of 5
    params.looping = False
    v.loopParams = params
    assert len(v.frames) == 5
    assert v.Eval(35) == 100.0

    # Test baking; start over
    v.clear()
    v.SetKeyFrame(Ts.KeyFrame(0.0, 1.0))
    v.SetKeyFrame(Ts.KeyFrame(1.0, 2.0))
    assert len(v.frames) == 2
    # Turn on looping
    v.loopParams = Ts.LoopParams(True, 0, 2, 2, 1, 2)
    assert len(v.frames) == 5
    assert v.loopParams.looping == True
    # grab keys before baking
    keysBeforeBaking = v[:]
    assert len(keysBeforeBaking) == 5
    # now bake
    v.BakeSplineLoops()
    # looping should be off
    assert v.loopParams.looping == False
    # and it should have the same keys
    assert keysBeforeBaking == v[:]

TestLooping()
