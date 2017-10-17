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
#ifndef HD_INSTANCER_H
#define HD_INSTANCER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

/// \class HdInstancer
///
/// This class exists to facilitate point cloud style instancing. Instancers,
/// conceptually, are instructions to draw N objects; for each object, store
/// which Rprim you're drawing and what instance-specific primvars you're
/// binding.
///
///     "/InstancerA": prototypes = ["/sphere", "/cube", "/sphere"]; 
///                    translate  = [  <0,0,0>, <1,0,0>,   <0,1,0>]
///
/// Hydra stores this in reverse: Rprims store which instancer is drawing them,
/// and the instancer stores which indices in that array of N objects are the
/// given Rprim.
///
///     "/sphere": instancerId = "/InstancerA"
///     "/cube": instancerId = "/InstancerA"
///     Instancer A: indices("/sphere") = [0, 2]
///                  indices("/cube") = [1]
///                  translate = [<0,0,0>, <1,0,0>, <0,1,0>]
///
/// Instancing is implemented by the prototype drawing itself multiple times,
/// and looking up per-instance data each time based on "indices": so
/// "/sphere" would draw itself once with translate=<0,0,0> and once with
/// translate=<0,1,0>.
///
/// To make things more exciting, instancers can be nested.
///
///     "/cube": instancerId = "/InstancerA"
///     "/InstancerA": instancerId = "/InstancerB"
///                    indices("/cube") = [0, 1]
///                    translate = [<0,0,0>, <1,0,0>]
///     "/InstancerB": indices("/InstancerA") = [0, 1]
///                    translate = [<0,0,0>, <0,1,0>]
///
/// In this case, "/cube" draws itself four times, for each of the
/// index tuples <0,0>, <0,1>, <1,0>, <1,1> where the first index is
/// the index in instancerA, and the second index is in instancerB.
///
/// If the same primvar (e.g. "translate") shows up at multiple levels of
/// nesting, it's resolved as follows:
///
/// Transforms
/// ----------
///
/// Instance primvars "translate", "rotate", "scale", and "instanceTransform"
/// are used to compute the final transform of an instance. "translate"
/// and "scale" are interpreted as vec3: position, and axis-aligned scale
/// respectively. "rotate" is interpreted as a vec4 quaternion, and
/// "instanceTransform" is a 4x4 matrix.  In the transform computation,
/// everything is converted to a 4x4 matrix.
///
/// There are additional transforms: "instancerTransform" comes from
/// HdSceneDelegate::GetInstancerTransform(instancer, proto), and represents
/// the constant transform between the instancer and the prototype. It
/// varies with each level of nesting, but not across instances.
///
/// "transform" is the proto Rprim's local transform.
///
/// The final instance transform for instance "index" is computed as:
///
///     nested_transform(level) = instancerTransform(level) *
///                               translate(level, index) *
///                               rotate(level, index) *
///                               scale(level, index) *
///                               instanceTransform(level, index);
///     output_transform = product(i : nested-levels - 1 -> 0) {
///                           nested_transform(i)
///                        } * transform;
///
/// Any transforms not provided by the scene delegate are set to identity.
///
/// Class responsibilities
/// ======================
///
/// HdInstancer's primary role is to track the "indices" arrays for each
/// proto used by an instancer, and any provided primvar arrays. The
/// implementation is in the renderer-specific instancers, like HdStInstancer.
///
/// All data access (aside from local caches) is delegated to the HdSceneDelegate.
///

class HdInstancer {
public:
    /// Constructor.
    HD_API
    HdInstancer(HdSceneDelegate* delegate, SdfPath const& id,
                SdfPath const &parentInstancerId);
    virtual ~HdInstancer() {}

    /// Returns the identifier.
    SdfPath const& GetId() const { return _id; }

    /// Returns the parent instancer identifier.
    SdfPath const& GetParentId() const { return _parentId; }

    HdSceneDelegate* GetDelegate() const { return _delegate; }

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;
    SdfPath _parentId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_INSTANCER_H
