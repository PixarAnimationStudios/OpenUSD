//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_INSTANCER_H
#define PXR_IMAGING_HD_INSTANCER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdRenderIndex;
class HdRprim;
class HdRenderParam;

/// \class HdInstancer
///
/// This class exists to facilitate point cloud style instancing. Instancers,
/// conceptually, are instructions to draw N objects; for each object, store
/// which Rprim you're drawing and what instance-specific primvars you're
/// binding.
///
///     "/InstancerA": prototypes = ["/sphere", "/cube", "/sphere"]; 
///                    hydra:instanceTranslations  = [<0,0,0>, <1,0,0>, <0,1,0>]
///
/// Hydra stores this in reverse: Rprims store which instancer is drawing them,
/// and the instancer stores which indices in that array of N objects are the
/// given Rprim.
///
///     "/sphere": instancerId = "/InstancerA"
///     "/cube": instancerId = "/InstancerA"
///     Instancer A: indices("/sphere") = [0, 2]
///                  indices("/cube") = [1]
///                  hydra:instanceTranslations = [<0,0,0>, <1,0,0>, <0,1,0>]
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
///                    hydra:instanceTranslations = [<0,0,0>, <1,0,0>]
///     "/InstancerB": indices("/InstancerA") = [0, 1]
///                    hydra:instanceTranslations = [<0,0,0>, <0,1,0>]
///
/// In this case, "/cube" draws itself four times, for each of the
/// index tuples <0,0>, <0,1>, <1,0>, <1,1> where the first index is
/// the index in instancerA, and the second index is in instancerB.
///
/// If the same primvar (e.g. "hydra:instanceTranslations") shows up at multiple
/// levels of nesting, it's resolved as follows:
///
/// Transforms
/// ----------
///
/// Instance primvars "hydra:instanceTranslations", "hydra:instanceRotations",
/// "hydra:instanceScales", and "hydra:instanceTransforms" are used to compute
/// the final transform of an instance. "hydra:instanceTranslations" and
/// "hydra:instanceScales" are interpreted as vec3: position, and axis-aligned
/// scale respectively. "hydra:instanceRotations" is interpreted as a vec4
/// quaternion (<real, i, j k>), and "hydra:instanceTransforms" is a 4x4 matrix.
/// In the transform computation, everything is converted to a 4x4 matrix.
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
///                               hydra:instanceTranslations(level, index) *
///                               hydra:instanceRotations(level, index) *
///                               hydra:instanceScales(level, index) *
///                               hydra:instanceTransforms(level, index);
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
/// All data access (aside from local caches) is routed to the HdSceneDelegate.
///

class HdInstancer {
public:
    /// Constructor.
    HD_API
    HdInstancer(HdSceneDelegate* delegate, SdfPath const& id);

    HD_API
    virtual ~HdInstancer();

    /// Returns the identifier.
    SdfPath const& GetId() const { return _id; }

    /// Returns the parent instancer identifier.
    SdfPath const& GetParentId() const { return _parentId; }

    HdSceneDelegate* GetDelegate() const { return _delegate; }

    HD_API
    static int GetInstancerNumLevels(HdRenderIndex& index,
                                     HdRprim const& rprim);

    HD_API
    static TfTokenVector const & GetBuiltinPrimvarNames();

    HD_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits);

    HD_API
    virtual void Finalize(HdRenderParam *renderParam);

    HD_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const;

    HD_API
    static void _SyncInstancerAndParents(
        HdRenderIndex &renderIndex,
        SdfPath const& instancerId);

protected:
    HD_API
    void _UpdateInstancer(HdSceneDelegate *delegate,
                          HdDirtyBits *dirtyBits);

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;
    SdfPath _parentId;

    // XXX: This mutex exists for _SyncInstancerAndParents, which will go
    // away when the render index calls sync on instancers.
    std::mutex _instanceLock;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_INSTANCER_H
