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
#include "pxr/imaging/hd/repr.h"
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

    // ---------------------------------------------------------------------- //
    /// \name Rprim Hydra Engine API : Pre-Sync & Sync-Phase
    // ---------------------------------------------------------------------- //

    /// Returns the set of dirty bits that should be
    /// added to the change tracker for this prim, when this prim is inserted.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const = 0;

    /// This function gives an Rprim the chance to "early exit" from dirty
    /// bit propagation, delegate sync and rprim sync altogether. It is
    /// a temporary measure to prevent unnecessary work, like in the case of
    /// invisible prims. The dirty bits in the change tracker remain the same.
    /// See the implementation for the finer details.
    HD_API
    bool CanSkipDirtyBitPropagationAndSync(HdDirtyBits bits) const;

    /// This function gives an Rprim the chance to set additional dirty bits
    /// based on those set in the change tracker, before passing the dirty bits
    /// to the scene delegate.
    /// It calls into _PropagateDirtyBits, which gives the Rprim an opportunity
    /// to specify the additional data needed to process the requested changes.
    ///
    /// The return value is the new set of dirty bits.
    HD_API
    HdDirtyBits PropagateRprimDirtyBits(HdDirtyBits bits);

    /// Initialize the representation of this Rprim by calling _InitRepr.
    /// This is called prior to dirty bit propagation & sync, the first time the
    /// repr is used, or when the authored representation is dirty.
    ///
    /// dirtyBits is an in/out value.  It is initialized to the dirty bits
    /// from the change tracker.  InitRepr can then set additional dirty bits
    /// if additional data is required from the scene delegate when this
    /// repr is synced. 
    HD_API
    void InitRepr(HdSceneDelegate* delegate,
                  TfToken const &reprToken,
                  HdDirtyBits *dirtyBits);

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///
    /// This function is told which scene data to pull through the
    /// dirtyBits parameter. The first time it's called, dirtyBits comes
    /// from _GetInitialDirtyBits(), which provides initial dirty state,
    /// but after that it's driven by invalidation tracking in the scene
    /// delegate.
    ///
    /// The contract for this function is that the prim can only pull on scene
    /// delegate buffers that are marked dirty. Scene delegates can and do
    /// implement just-in-time data schemes that mean that pulling on clean
    /// data will be at best incorrect, and at worst a crash.
    ///
    /// This function is called in parallel from worker threads, so it needs
    /// to be threadsafe; calls into HdSceneDelegate are ok.
    ///
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam A render delegate object that holds rendering
    ///                      parameters that scene geometry may use.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///   \param reprToken The representation that needs to be updated. This is
    ///                    useful for backends that support multiple display
    ///                    representations for an rprim. A given representation
    ///                    may choose to pull on a subset of the dirty state.
    ///   \param dirtyBits On input specifies which state is dirty and can be
    ///                    pulled from the scene delegate.
    ///                    On output specifies which bits are still dirty and
    ///                    were not cleaned by the sync.
    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken) = 0;
    // ---------------------------------------------------------------------- //
    /// \name Rprim Hydra Engine API : Execute Phase
    // ---------------------------------------------------------------------- //

    /// Returns the draw items for the requested repr token, if any.
    /// These draw items should be constructed and cached beforehand by Sync().
    /// If no draw items exist, or reprToken cannot be found, nullptr will be
    /// returned.
    using HdDrawItemPtrVector = std::vector<HdDrawItem*>;
    HD_API
    const HdDrawItemPtrVector*
    GetDrawItems(TfToken const& reprToken) const;

    // ---------------------------------------------------------------------- //
    /// \name Rprim Hydra Engine API : Cleanup
    // ---------------------------------------------------------------------- //
    /// Finalizes object resources. This function might not delete resources,
    /// but it should deal with resource ownership so that the rprim is
    /// deletable.
    HD_API
    virtual void Finalize(HdRenderParam *renderParam);

    // ---------------------------------------------------------------------- //
    /// \name Rprim Data API
    // ---------------------------------------------------------------------- //

    /// Returns the identifier of this Rprim. This is both used in the
    /// RenderIndex and the SceneDelegate and acts as the associative key for
    /// the Rprim in both contexts.
    SdfPath const& GetId() const { return _sharedData.rprimID; }

    /// Return the unique instance id
    int32_t GetPrimId() const { return _primId; };

    /// Set the unique instance id
    HD_API
    void SetPrimId(int32_t primId);

    /// Returns the identifier of the instancer (if any) for this Rprim. If this
    /// Rprim is not instanced, an empty SdfPath will be returned.
    SdfPath const& GetInstancerId() const { return _instancerId; }

    /// Returns the path of the material to which this Rprim is bound. The
    /// material object itself can be fetched from the RenderIndex using
    /// this identifier.
    SdfPath const& GetMaterialId() const { return _materialId; }

    HdReprSelector const& GetReprSelector() const {
        return _authoredReprSelector;
    }

    /// Returns the render tag associated to this rprim
    inline  TfToken GetRenderTag(HdSceneDelegate* delegate) const;

    /// Returns the bounds of the rprim in local, untransformed space.
    inline GfRange3d GetExtent(HdSceneDelegate* delegate) const;

    /// Primvar Query
    inline HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdSceneDelegate* delegate,
                          HdInterpolation interpolation) const;

    inline VtValue
    GetPrimvar(HdSceneDelegate* delegate, const TfToken &name) const;

    /// Returns true if any dirty flags are set for this rprim.
    HD_API
    bool IsDirty(HdChangeTracker &changeTracker) const;

    /// Is the prim itself visible
    bool IsVisible() const { return _sharedData.visible; }

    HD_API
    void UpdateReprSelector(HdSceneDelegate* delegate,
                            HdDirtyBits *dirtyBits);

protected:
    // ---------------------------------------------------------------------- //
    /// \name Rprim Hydra Engine API : Pre-Sync & Sync-Phase
    // ---------------------------------------------------------------------- //

    /// This callback from Rprim gives the prim an opportunity to set
    /// additional dirty bits based on those already set.  This is done
    /// before the dirty bits are passed to the scene delegate, so can be
    /// used to communicate that extra information is needed by the prim to
    /// process the changes.
    ///
    /// The return value is the new set of dirty bits, which replaces the bits
    /// passed in.
    ///
    /// See HdRprim::PropagateRprimDirtyBits()
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const = 0;

    /// Initialize the given representation of this Rprim.
    /// This is called prior to syncing the prim, the first time the repr
    /// is used.
    ///
    /// reprToken is the name of the representation to initalize.
    ///
    /// dirtyBits is an in/out value.  It is initialized to the dirty bits
    /// from the change tracker.  InitRepr can then set additional dirty bits
    /// if additional data is required from the scene delegate when this
    /// repr is synced.  InitRepr occurs before dirty bit propagation.
    ///
    /// See HdRprim::InitRepr()
       virtual void _InitRepr(TfToken const &reprToken,
                              HdDirtyBits *dirtyBits) = 0;

    // ---------------------------------------------------------------------- //
    /// \name Rprim Shared API
    // ---------------------------------------------------------------------- //
    HD_API
    HdReprSharedPtr const & _GetRepr(TfToken const &reprToken) const;

    HD_API
    void _UpdateVisibility(HdSceneDelegate *sceneDelegate,
                           HdDirtyBits *dirtyBits);

    /// Sets a new material binding to be used by this rprim
    HD_API
    void _SetMaterialId(HdChangeTracker &changeTracker,
                        SdfPath const& materialId);

    /// note: constant range has to be shared across reprs (smooth, refined),
    /// since we're tracking dirtiness in a single bit (e.g. DirtyTransform)
    /// unlike vertex primvars (DirtyPoints-DirtyRefinedPoints)
    HD_API
    void _PopulateConstantPrimvars(HdSceneDelegate *sceneDelegate,
                            HdDrawItem *drawItem,
                            HdDirtyBits *dirtyBits,
                            HdPrimvarDescriptorVector const &constantPrimvars);

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

private:
    SdfPath _instancerId;
    SdfPath _materialId;

    // Used for id renders.
    int32_t _primId;

protected:
    // shared data across reprs: bufferArrayRanges, bounds, visibility
    HdRprimSharedData _sharedData;

    // authored repr selector
    HdReprSelector _authoredReprSelector;

    // total number of reprs is relatively small (less than 5 or so
    // in most case), we use linear container for efficiency.
    typedef std::vector<std::pair<TfToken, HdReprSharedPtr> >
        _ReprVector;
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
    // have this static map to lookup descriptors for the given reprToken.
    //
    // N : # of descriptors for the repr.
    //
    template<typename DESC_TYPE, int N=1>
    struct _ReprDescConfigs {
        typedef std::array<DESC_TYPE, N> DescArray;
        static const int MAX_DESCS = N;

        DescArray Find(TfToken const &reprToken) const {
            // linear search, we expect only a handful reprs configured.
            TF_FOR_ALL (it, _configs) {
                if (it->first == reprToken) return it->second;
            }
            TF_CODING_ERROR("Repr %s not found", reprToken.GetText());
            return DescArray();
        }
        void AddOrUpdate(TfToken const &reprToken, DescArray descs) {
            for (auto& config : _configs) {
                if (config.first == reprToken) {
                    // Overrwrite the existing entry.
                    config.second = descs;
                    return;
                }
            }
            _configs.push_back(std::make_pair(reprToken, descs));
        }
        std::vector<std::pair<TfToken, DescArray> > _configs;
    };

};

////////////////////////////////////////////////////////////////////////////////
//
// Delegate API Wrappers
//
TfToken
HdRprim::GetRenderTag(HdSceneDelegate* delegate) const
{
    return delegate->GetRenderTag(GetId());
}

GfRange3d
HdRprim::GetExtent(HdSceneDelegate* delegate) const
{
    return delegate->GetExtent(GetId());
}

inline HdPrimvarDescriptorVector
HdRprim::GetPrimvarDescriptors(HdSceneDelegate* delegate,
                               HdInterpolation interpolation) const
{
    return delegate->GetPrimvarDescriptors(GetId(), interpolation);
}

inline VtValue
HdRprim::GetPrimvar(HdSceneDelegate* delegate, const TfToken &name) const
{
    return delegate->Get(GetId(), name);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RPRIM_H
