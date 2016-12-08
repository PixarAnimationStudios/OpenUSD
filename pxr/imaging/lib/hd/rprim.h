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
#ifndef HD_RPRIM_H
#define HD_RPRIM_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/shaderKey.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/gf/range3d.h"

#include <boost/shared_ptr.hpp>

class HdDrawItem;
class HdRenderIndex;
class HdRepr;
class HdSceneDelegate;

typedef boost::shared_ptr<HdRepr> HdReprSharedPtr;

/// \class HdRprim
///
/// The render engine state for a given rprim from the scene graph. All data 
/// access (aside from local caches) is delegated to the HdSceneDelegate.
///
class HdRprim {
public:
    HdRprim(HdSceneDelegate* delegate,
            SdfPath const& id,
            SdfPath const& instancerId);
    virtual ~HdRprim();

    /// Returns the draw items for the requested reprName, these draw items
    /// should be constructed and cached beforehand by Sync().
    std::vector<HdDrawItem>* GetDrawItems(TfToken const &reprName, bool forced);

    /// Update objects representation based on dirty bits.
    void Sync(TfToken const &reprName, bool forced,
              HdChangeTracker::DirtyBits *dirtyBits);

    /// Returns the bounds of the rprim in local, untransformed space.
    GfRange3d GetExtent();

    /// Returns true if the rprim exists in the named collection.
    bool IsInCollection(TfToken const& collectionName) const;

    /// Returns the identifier of this Rprim. This is both used in the
    /// RenderIndex and the SceneDelegate and acts as the associative key for
    /// the Rprim in both contexts.
    SdfPath const& GetId() const { return _id; }

    /// Returns the identifier of the instancer (if any) for this Rprim. If this
    /// Rprim is not instanced, an empty SdfPath will be returned.
    SdfPath const& GetInstancerId() const { return _instancerID; }

    /// Returns the ID of the SurfaceShader to which this Rprim is bound. The
    /// SurfaceShader object itself can be fetched from the RenderIndex using
    /// this identifier.
    SdfPath const& GetSurfaceShaderId() const { 
        return _surfaceShaderID;
    }

    /// Sets a new surface shader id to be used by this rprim
    void SetSurfaceShaderId(SdfPath const& surfaceShaderId);

    /// Returns true if any dirty flags are set for this rprim.
    bool IsDirty();

    /// Set the unique instance id
    void SetPrimId(int32_t primId);

    /// Return the unique instance id
    int32_t GetPrimId() const { return _primId; };

    /// Return the dirtyBits mask to be tracked for rprim.
    static int GetDirtyBitsMask(TfToken const &reprName);

    int GetInitialDirtyBitsMask() const;

    /// Returns the SceneDelegate object that is backing this Rprim.
    HdSceneDelegate* GetDelegate();

    /// Returns the SceneDelegate object that is backing this Rprim.
    /// Note: The scene delegate returned is non-const as the scene
    /// delegate may need to perform mutable operations.
    HdSceneDelegate* GetDelegate() const;

protected:
    virtual HdReprSharedPtr const & _GetRepr(
        TfToken const &reprName, HdChangeTracker::DirtyBits *dirtyBits) = 0;

    void _UpdateVisibility(HdChangeTracker::DirtyBits *dirtyBits);
    HdRenderIndex& _GetRenderIndex();
    HdRenderIndex const& _GetRenderIndex() const;
    HdChangeTracker& _GetChangeTracker();

    /// note: constant range has to be shared across reprs (smooth, refined),
    /// since we're tracking dirtiness in a single bit (e.g. DirtyTransform)
    /// unlike vertex primvars (DirtyPoints-DirtyRefinedPoints)
    void _PopulateConstantPrimVars(HdDrawItem *drawItem,
                                   HdChangeTracker::DirtyBits *dirtyBits);

    void _PopulateInstancePrimVars(HdDrawItem *drawItem,
                                   HdChangeTracker::DirtyBits *dirtyBits,
                                   int instancePrimVarSlot);

    VtMatrix4dArray _GetInstancerTransforms();

    TfToken _GetReprName(TfToken const &defaultReprName, bool forced,
                         HdChangeTracker::DirtyBits *dirtyBits);

    virtual HdChangeTracker::DirtyBits _GetInitialDirtyBits() const = 0;

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;
    SdfPath _instancerID;
    SdfPath _surfaceShaderID;

    // Used for id renders.
    int32_t _primId;

protected:
    // shared data across reprs: bufferArrayRanges, bounds, visibility
    HdRprimSharedData _sharedData;

    // authored repr name
    TfToken _authoredReprName;

    // total number of reprs is relatively small (less than 5 or so
    // in most case), we use linear container for efficiency.
    typedef std::vector<std::pair<TfToken, HdReprSharedPtr> > _ReprVector;
    _ReprVector _reprs;

    struct _ReprComparator {
        _ReprComparator(TfToken const &name) : _name(name) {}
        bool operator() (const std::pair<TfToken, HdReprSharedPtr> &e) const {
            return _name == e.first;
        }
    private:
        TfToken _name;
    };

    // Repr configuration descriptors. All concrete types (HdMesh, HdPoints ..)
    // have this static map to lookup descriptors for the given reprname.
    //
    // N : # of drawItems to be created for the reprname (# of descriptors)
    //
    template<typename DESC_TYPE, int N=1>
    struct _ReprDescConfigs {
        typedef std::array<DESC_TYPE, N> DescArray;
        static const int MAX_DESCS = N;

        DescArray Find(TfToken const &reprName) const {
            // linear search, we expect only a handful reprs configured.
            TF_FOR_ALL (it, _configs) {
                if (it->first == reprName) return it->second;
            }
            TF_CODING_ERROR("Repr %s not found", reprName.GetText());
            return DescArray();
        }
        void Append(TfToken const &reprName, DescArray descs) {
            _configs.push_back(std::make_pair(reprName, descs));
        }
        std::vector<std::pair<TfToken, DescArray> > _configs;
    };

};

#endif //HD_RPRIM_H
