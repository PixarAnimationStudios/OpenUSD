#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

def main():
    shotFilePath = 'shots/s00_01/s00_01.usd'
    animLayerFilePath = 'shots/s00_01/s00_01_anim.usd'

    from pxr import Usd, Sdf
    stage = Usd.Stage.Open(shotFilePath)

    # set the timeCode range for the shot that other applications can use.
    stage.SetStartTimeCode(1)
    stage.SetEndTimeCode(10)

    stage.GetRootLayer().Save()

    # we use Sdf, a lower level library, to obtain the 'anim' layer.
    workingLayer = Sdf.Layer.FindOrOpen(animLayerFilePath)
    assert stage.HasLocalLayer(workingLayer)

    # this makes the workingLayer the target for authoring operations by the
    # stage.
    stage.SetEditTarget(workingLayer)

    _AnimateBilliards(stage)

    stage.GetEditTarget().GetLayer().Save()

    print('===')
    print('usdview %s' % shotFilePath)
    print('usdcat %s' % animLayerFilePath)

def _AnimateBilliards(stage):
    _MoveBall(stage.GetPrimAtPath('/World/anim/CueBall'), (1, 0), 40)
    #_MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_2'), (0, 0), 0.0)
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_10'), (0, -1), 20)
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_1'), (1, -1), 5)
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_4'), (1, -0.5), 3.6)

def _MoveBall(ball, direction, speed):

    from pxr import Usd, UsdGeom, Gf
    v = Gf.Vec2d(*direction).GetNormalized() * speed
    xformAPI = UsdGeom.XformCommonAPI(ball)
    # Here we make use of GfVec3d rather than plain tuples so we can
    # add vectors, and demonstrate the API (all Usd API's) will accept either.
    # GetXformVectors() returns a tuple of 
    # (translation, rotation, scale, pivot, rotationOrder), from which we
    # extract the first element, the translation
    start = xformAPI.GetXformVectors(Usd.TimeCode.Default())[0]
    end = start + Gf.Vec3d(v[0], 0, v[1])

    # set keyframe for frame 1 and frame 10.
    xformAPI.SetTranslate(start, 1)
    xformAPI.SetTranslate(end, 10)
    xformAPI.SetRotate(speed * Gf.Vec3f(3.7,  13.9, 91.34))
    
if __name__ == '__main__':
    main()

