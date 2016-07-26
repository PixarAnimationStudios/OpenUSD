#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#

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

    print '==='
    print 'usdview %s' % shotFilePath
    print 'usdcat %s' % animLayerFilePath

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

