//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/tf/pyModule.h"

TF_WRAP_MODULE
{
    TF_WRAP( BBox3d );    
    TF_WRAP( Frustum );
    TF_WRAP( Gamma );
    TF_WRAP( Half );
    TF_WRAP( Homogeneous );
    TF_WRAP( Interval );
    TF_WRAP( Limits );
    TF_WRAP( Line );
    TF_WRAP( LineSeg );
    TF_WRAP( Math );
    TF_WRAP( MultiInterval );

    TF_WRAP( Matrix2d );
    TF_WRAP( Matrix2f );
    TF_WRAP( Matrix3d );
    TF_WRAP( Matrix3f );
    TF_WRAP( Matrix4f );
    TF_WRAP( Matrix4d );

    TF_WRAP( Plane );
    TF_WRAP( Quatf );
    TF_WRAP( Quatd );
    TF_WRAP( Quath );
    TF_WRAP( Quaternion );
    TF_WRAP( Ray );

    // Order of wrapping Ranges matters because in the cases where overloads
    // could choose either float or double, we want the double versions to be
    // found first and preferred so we wrap them last.
    TF_WRAP( Range1f );
    TF_WRAP( Range1d );
    TF_WRAP( Range2f );
    TF_WRAP( Range2d );
    TF_WRAP( Range3f );
    TF_WRAP( Range3d );

    TF_WRAP( Rect2i );
    TF_WRAP( Rotation );
    TF_WRAP( Size2 );
    TF_WRAP( Size3 );

    // Order of wrapping Vecs matters because in the cases where overloads could
    // choose either float or double, we want the double versions to be found
    // first and preferred so we wrap them last.
    TF_WRAP( Vec2h );
    TF_WRAP( Vec2f );
    TF_WRAP( Vec2d );
    TF_WRAP( Vec2i );

    TF_WRAP( Vec3h );
    TF_WRAP( Vec3f );
    TF_WRAP( Vec3d );
    TF_WRAP( Vec3i );
    
    TF_WRAP( Vec4h );
    TF_WRAP( Vec4f );
    TF_WRAP( Vec4d );
    TF_WRAP( Vec4i );


    // Note that Transform must be wrapped after Rotation and Vec3d so that it
    // can create python objects for them as keyword args.
    TF_WRAP( Transform );


    TF_WRAP( Camera );
}
