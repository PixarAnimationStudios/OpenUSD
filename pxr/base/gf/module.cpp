//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB( BBox3d );
    TF_WRAP_NB( Color );
    TF_WRAP_NB( ColorSpace );
    TF_WRAP_NB( DualQuatd );
    TF_WRAP_NB( DualQuatf );
    TF_WRAP_NB( DualQuath );
    TF_WRAP_NB( Duration );
    TF_WRAP_NB( Frustum );
    TF_WRAP_NB( Gamma );
    TF_WRAP_NB( Half );
    TF_WRAP_NB( Homogeneous );
    TF_WRAP_NB( Interval );
    TF_WRAP_NB( Limits );
    TF_WRAP_NB( Line );
    TF_WRAP_NB( LineSeg );
    TF_WRAP_NB( Math );
    TF_WRAP_NB( MultiInterval );

    TF_WRAP_NB( Matrix2d );
    TF_WRAP_NB( Matrix2f );
    TF_WRAP_NB( Matrix3d );
    TF_WRAP_NB( Matrix3f );
    TF_WRAP_NB( Matrix4d );
    TF_WRAP_NB( Matrix4f );

    TF_WRAP_NB( Plane );
    TF_WRAP_NB( Quatd );
    TF_WRAP_NB( Quatf );
    TF_WRAP_NB( Quath );
    TF_WRAP_NB( Quaternion );
    TF_WRAP_NB( Ray );

    // Order of wrapping Ranges matters because in the cases where overloads
    // could choose either float or double, we want the double versions to be
    // found first and preferred so in nanobind (FIFO) we wrap them first.
    TF_WRAP_NB( Range1d );
    TF_WRAP_NB( Range1f );
    TF_WRAP_NB( Range2d );
    TF_WRAP_NB( Range2f );
    TF_WRAP_NB( Range3d );
    TF_WRAP_NB( Range3f );

    TF_WRAP_NB( Rect2i );
    TF_WRAP_NB( Rotation );
    TF_WRAP_NB( TimeCode );

    // Order of wrapping Vecs matters because in the cases where overloads could
    // choose either float or double, we want the double versions to be found
    // first and preferred so in nanobind (FIFO) we wrap them first.
    TF_WRAP_NB( Vec2i );
    TF_WRAP_NB( Vec2d );
    TF_WRAP_NB( Vec2f );
    TF_WRAP_NB( Vec2h );

    TF_WRAP_NB( Vec3i );
    TF_WRAP_NB( Vec3d );
    TF_WRAP_NB( Vec3f );
    TF_WRAP_NB( Vec3h );
    
    TF_WRAP_NB( Vec4i );
    TF_WRAP_NB( Vec4d );
    TF_WRAP_NB( Vec4f );
    TF_WRAP_NB( Vec4h );

    // Size2 and Size3 register nanobind::implicitly_convertible to GfVec2i /
    // GfVec3i, which requires GfVec2i and GfVec3i to already be registered.
    TF_WRAP_NB( Size2 );
    TF_WRAP_NB( Size3 );

    // Note that Transform must be wrapped after Rotation and Vec3d so that it
    // can create python objects for them as keyword args.
    TF_WRAP_NB( Transform );
    TF_WRAP_NB( Traits );

    TF_WRAP_NB( Camera );
}
