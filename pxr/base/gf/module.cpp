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
    TF_WRAP( BBox3d );
    TF_WRAP( Color );
    TF_WRAP( ColorSpace );
    TF_WRAP( DualQuatd );
    TF_WRAP( DualQuatf );
    TF_WRAP( DualQuath );
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
    TF_WRAP( Quatd );
    TF_WRAP( Quatf );
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
