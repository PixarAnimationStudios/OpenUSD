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

import os
ASSET_BASE = os.path.join(os.getcwd(), 'models')
TABLE_HEIGHT = 74.5
BALL_RADIUS = 5.715 * 0.5

def main():
    shotFilePath = 'shots/s00_01/s00_01.usd'
    layoutLayerFilePath = 'shots/s00_01/s00_01_layout.usd'

    from pxr import Usd, Sdf
    stage = Usd.Stage.Open(shotFilePath)

    # set the timeCode range for the shot that other applications can use.
    stage.SetStartTimeCode(1)
    stage.SetEndTimeCode(10)

    # we use Sdf, a lower level library, to obtain the 'layout' layer.
    workingLayer = Sdf.Layer.FindOrOpen(layoutLayerFilePath)
    assert stage.HasLocalLayer(workingLayer)

    # this makes the workingLayer the target for authoring operations by the
    # stage.
    stage.SetEditTarget(workingLayer)

    _SetupBilliards(stage)
    _MoveCamera(stage)

    stage.GetEditTarget().GetLayer().Save()

    print '==='
    print 'usdview %s' % shotFilePath
    print 'usdcat %s' % layoutLayerFilePath

def _SetupBilliards(stage):
    from pxr import Kind, Sdf, Usd, UsdGeom

    # Make sure the model-parents we need are well-specified
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World')).SetKind(Kind.Tokens.group)
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World/anim')).SetKind(Kind.Tokens.group)
    # in previous examples, we've been using GetReferences().AddReference(...).  The
    # following uses .SetItems() instead which lets us explicitly set (replace)
    # the references at once instead of adding.
    cueBall = stage.DefinePrim('/World/anim/CueBall')
    cueBall.GetReferences().SetReferences([
        Sdf.Reference(os.path.join(ASSET_BASE, 'Ball/Ball.usd'))])

    # deactivate everything that isn't 8, 9, 1, 4.  We accumulate the prims we
    # want to deactivate so that we don't delete while iterating.
    roomProps = stage.GetPrimAtPath('/World/sets/Room_set/Props')
    keepers = set(['Ball_%d' % i for i in [1, 9, 8, 4] ])
    toDeactivate = []
    for child in roomProps.GetChildren():
        if child.GetName() not in keepers:
            toDeactivate.append(child)
    for prim in toDeactivate:
        prim.SetActive(False)

    # the offset values are relative to the radius of the ball
    def _MoveBall(ballPrim, offset):
        translation = (offset[0] * BALL_RADIUS,
                       TABLE_HEIGHT + BALL_RADIUS,
                       offset[1] * BALL_RADIUS)

        # Apply the UsdGeom.Xformable schema to the model, and then set the
        # transformation.  Note we can use ordinary python tuples
        from pxr import UsdGeom
        UsdGeom.XformCommonAPI(ballPrim).SetTranslate(translation)

    # all these values are just eye-balled and are relative to Ball_1.
    _MoveBall(cueBall, (1.831,  4.331))
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_2'), (0.000,  0.000))
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_10'), (2.221,  1.119))
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_1'), (3.776,  0.089))
    _MoveBall(stage.GetPrimAtPath('/World/sets/Room_set/Props/Ball_4'), (5.453, -0.543))

def _MoveCamera(stage):
    from pxr import UsdGeom, Gf
    cam = UsdGeom.Camera.Get(stage, '/World/main_cam')

    # the camera derives from UsdGeom.Xformable so we can 
    # use the XformCommonAPI on it, too, and see how rotations are handled
    xformAPI = UsdGeom.XformCommonAPI(cam)
    xformAPI.SetTranslate( (8, 120, 8) )
    # -86 degree rotation around X axis.  Can specify rotation order as
    # optional parameter
    xformAPI.SetRotate( (-86, 0, 0 ) )

if __name__ == '__main__':
    main()

