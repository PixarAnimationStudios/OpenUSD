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

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/arch/inttypes.h"

#include <boost/shared_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferSource;
class HdDrawItem;
class HdMaterial;
class HdRenderIndex;
class HdRepr;
class HdRenderParam;

typedef boost::shared_ptr<HdRepr> HdReprSharedPtr;
typedef boost::shared_ptr<HdBufferSource> HdBufferSourceSharedPtr;

typedef std::vector<struct HdBufferSpec> HdBufferSpecVector;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;
typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceVector;
typedef boost::shared_ptr<HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::shared_ptr<class HdComputation> HdComputationSharedPtr;
typedef std::vector<HdComputationSharedPtr> HdComputationVector;

/// \class HdRprim
///
/// The render engine state for a given rprim from the scene graph. All data
/// access (aside from local caches) is delegated to the HdSceneDelegate.
///
class HdRprim {
public:
    HD_API
    HdRprim(SdfPath const& id,
            SdfPath const& instancerId);
    HD_API
    virtual ~HdRprim();

    /// Update objects representation based on dirty bits.
    /// @param[in, out]  dirtyBits: On input specifies which state is
    ///                             is dirty and can be pulled from the scene
    ///                             delegate.
    ///                             On output specifies which bits are still
    ///                             dirty and were not cleaned by the sync.
    ///
    virtual void Sync(HdSceneDelegate* delegate,
                      HdRenderParam*   renderParam,
                      HdDirtyBits*     dirtyBits,
                      TfToken const&   reprName,
                      bool             forcedRepr) = 0;

    /// Finalizes object resources. This function might not delete resources,
    /// but it should deal with resource ownership so that the rprim is deletable.
    HD_API
    virtual void Finalize(HdRenderParam *renderParam);

    /// Returns the draw items for the requested reprName, if any.
    /// These draw items should be constructed and cached beforehand by Sync().
    /// If no draw items exist, or reprName cannot be found, nullptr
    /// will be returned.
    HD_API
    const std::vector<HdDrawItem*>* GetDrawItems(TfToken const &reprName,
                                                 bool forced) const;

    /// Returns the render tag associated to this rprim
    HD_API
    TfToken GetRenderTag(HdSceneDelegate* delegate, TfToken const& reprName) const;

    /// Returns the identifier of this Rprim. This is both used in the
    /// RenderIndex and the SceneDelegate and acts as the associative key for
    /// the Rprim in both contexts.
    SdfPath const& GetId() const { return _id; }

    /// Returns the identifier of the instancer (if any) for this Rprim. If this
    /// Rprim is not instanced, an empty SdfPath will be returned.
    SdfPath const& GetInstancerId() const { return _instancerId; }

    /// Returns the path of the material to which this Rprim is bound. The
    /// material object itself can be fetched from the RenderIndex using
    /// this identifier.
    SdfPath const& GetMaterialId() const {
        return _materialId;
    }

    /// Returns true if any dirty flags are set for this rprim.
    HD_API
    bool IsDirty(HdChangeTracker &changeTracker) const;

    /// Set the unique instance id
    HD_API
    void SetPrimId(int32_t primId);

    /// Return the unique instance id
    int32_t GetPrimId() const { return _primId; };

    /// Is the prim itself visible
    bool IsVisible() const { return _sharedData.visible; }

    /// This function gives an Rprim the chance to "early exit" from dirty
    /// bit propagation, delegate sync and rprim sync altogether. It is
    /// a temporary measure to prevent unnecessary work, like in the case of
    /// invisible prims. The dirty bits in the change tracker remain the same.
    /// See the implementation for the finer details.
    HD_API
    bool CanSkipDirtyBitPropagationAndSync(HdDirtyBits bits) const;

    /// Returns the set of dirty bits that should be
    /// added to the change tracker for this prim, when this prim is inserted.
    HD_API
    HdDirtyBits GetInitialDirtyBitsMask() const;

    /// This function gives an Rprim the chance to set additional dirty bits
    /// based on those set in the change tracker, before passing the dirty bits
    /// to the scene delegate.  This gives the Rprim an opportunity to specify
    /// it needs more data to process the requested changes.
    ///
    /// The return value is the new set of dirty bits.
    HD_API
    HdDirtyBits PropagateRprimDirtyBits(HdDirtyBits bits);

    /// Initialize the given representation of this Rprim.
    /// This is called prior to syncing the prim, the first time the repr
    /// is used.
    ///
    /// defaultReprName is the name of the repr to initalize.  However, this
    /// can be overridden by the prim itself with an authored repr, which
    /// is obtained from the scene delegate.  If the forced flag is set
    /// the defaultReprName should be used, even if there is an authored repr.
    ///
    /// dirtyBits is an in/out value.  It is initialized to the dirty bits
    /// from the change tracker.  InitRepr can then set additional dirty bits
    /// if additional data is required from the scene delegate when this
    /// repr is synced.  InitRepr occurs before dirty bit propagation.
    HD_API
    void InitRepr(HdSceneDelegate* delegate,
                  TfToken const &defaultReprName,
                  bool forced,
                  HdDirtyBits *dirtyBits);



    /// Returns the bounds of the rprim in local, untransformed space.
    inline GfRange3d GetExtent(HdSceneDelegate* delegate) const;

    ///
    /// Primvar Query
    ///
    inline TfTokenVector GetPrimVarVertexNames(HdSceneDelegate* delegate)      const;
    inline TfTokenVector GetPrimVarVaryingNames(HdSceneDelegate* delegate)     const;
    inline TfTokenVector GetPrimVarFacevaryingNames(HdSceneDelegate* delegate) const;
    inline TfTokenVector GetPrimVarUniformNames(HdSceneDelegate* delegate)     const;
    inline TfTokenVector GetPrimVarConstantNames(HdSceneDelegate* delegate)    const;

    inline VtValue GetPrimVar(HdSceneDelegate* delegate, const TfToken &name)  const;

protected:
    /// Update objects representation based on dirty bits.
    HD_API
    void _Sync(HdSceneDelegate* delegate,
              TfToken const &reprName, bool forced,
              HdDirtyBits *dirtyBits);

    HD_API
    HdReprSharedPtr const & _GetRepr(TfToken const &reprName) const;

    virtual void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                             TfToken const &reprName,
                             HdDirtyBits *dirtyBits) = 0;

    HD_API
    void _UpdateVisibility(HdSceneDelegate *sceneDelegate,
                           HdDirtyBits *dirtyBits);

    /// note: constant range has to be shared across reprs (smooth, refined),
    /// since we're tracking dirtiness in a single bit (e.g. DirtyTransform)
    /// unlike vertex primvars (DirtyPoints-DirtyRefinedPoints)
    HD_API
    void _PopulateConstantPrimVars(HdSceneDelegate *sceneDelegate,
                                   HdDrawItem *drawItem,
                                   HdDirtyBits *dirtyBits);

    HD_API
    VtMatrix4dArray _GetInstancerTransforms(HdSceneDelegate* delegate);

    // methods to assist allocating and migrating shared primvar ranges
    HD_API
    static bool _IsEnabledSharedVertexPrimvar();

    HD_API
    uint64_t
    _ComputeSharedPrimvarId(uint64_t baseId,
                      HdBufferSourceVector const &sources,
                      HdComputationVector const &computations) const;

    HD_API
    TfToken _GetReprName(TfToken const &defaultReprName, bool forced) const;

    HD_API
    void _UpdateReprName(HdSceneDelegate* delegate, HdDirtyBits *dirtyBits);

    virtual HdDirtyBits _GetInitialDirtyBits() const = 0;
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const = 0;

    virtual void _InitRepr(TfToken const &reprName, HdDirtyBits *dirtyBits) = 0;

private:
    SdfPath _id;
    SdfPath _instancerId;
    SdfPath _materialId;

    // Used for id renders.
    int32_t _primId;

    /// Sets a new material binding to be used by this rprim
    void _SetMaterialId(HdChangeTracker &changeTracker,
                        SdfPath const& materialId);

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

////////////////////////////////////////////////////////////////////////////////
//
// Delegate API Wrappers
//

GfRange3d
HdRprim::GetExtent(HdSceneDelegate* delegate) const
{
    return delegate->GetExtent(GetId());
}

inline TfTokenVector
HdRprim::GetPrimVarVertexNames(HdSceneDelegate* delegate) const
{
    return delegate->GetPrimVarVertexNames(GetId());
}

inline TfTokenVector
HdRprim::GetPrimVarVaryingNames(HdSceneDelegate* delegate) const
{
    return delegate->GetPrimVarVaryingNames(GetId());
}

inline TfTokenVector
HdRprim::GetPrimVarFacevaryingNames(HdSceneDelegate* delegate) const
{
    return delegate->GetPrimVarFacevaryingNames(GetId());
}

inline TfTokenVector
HdRprim::GetPrimVarUniformNames(HdSceneDelegate* delegate) const
{
    return delegate->GetPrimVarUniformNames(GetId());
}

inline TfTokenVector
HdRprim::GetPrimVarConstantNames(HdSceneDelegate* delegate) const
{
    return delegate->GetPrimVarConstantNames(GetId());
}

inline VtValue
HdRprim::GetPrimVar(HdSceneDelegate* delegate, const TfToken &name) const
{
    return delegate->Get(GetId(), name);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RPRIM_H
