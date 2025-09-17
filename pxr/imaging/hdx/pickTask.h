//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_PICK_TASK_H
#define PXR_IMAGING_HDX_PICK_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/primOriginSchema.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/usd/sdf/path.h"

#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

#define HDX_PICK_TOKENS              \
    /* Task context */               \
    (pickParams)                     \
                                     \
    /* Pick target */                \
    (pickPrimsAndInstances)          \
    (pickFaces)                      \
    (pickEdges)                      \
    (pickPoints)                     \
    (pickPointsAndInstances)         \
                                     \
    /* Resolve mode */               \
    (resolveNearestToCamera)         \
    (resolveNearestToCenter)         \
    (resolveUnique)                  \
    (resolveAll)                     \
    (resolveDeep)

TF_DECLARE_PUBLIC_TOKENS(HdxPickTokens, HDX_API, HDX_PICK_TOKENS);

class HdStRenderBuffer;
class HdStRenderPassState;
using HdStShaderCodeSharedPtr = std::shared_ptr<class HdStShaderCode>;

class Hgi;

/// Pick task params. This contains render-style state (for example), but is
/// augmented by HdxPickTaskContextParams, which is passed in on the task
/// context.
struct HdxPickTaskParams
{
    HdxPickTaskParams()
        : cullStyle(HdCullStyleNothing)
        , enableSceneMaterials(true)
    {}

    HdCullStyle cullStyle;
    bool enableSceneMaterials;
};

/// Picking hit structure. This is output by the pick task as a record of
/// what objects the picking query found.
struct HdxPickHit
{
    /// delegateID of HdSceneDelegate that provided the picked prim.
    /// Irrelevant for scene indices.
    SdfPath delegateId;
    /// Path computed from scenePath's in primOrigin data source of
    /// picked prim and instancers if provided by scene index.
    /// Otherwise, path in render index.
    SdfPath objectId;
    /// Only supported for scene delegates, see HdxPrimOriginInfo for
    /// scene indices.
    SdfPath instancerId;
    int instanceIndex;
    int elementIndex;
    int edgeIndex;
    int pointIndex;
    GfVec3d worldSpaceHitPoint;
    GfVec3f worldSpaceHitNormal;
    /// normalizedDepth is in the range [0,1].  Nb: the pick depth buffer won't
    /// contain items drawn with materialTag "displayInOverlay" for simplicity.
    float normalizedDepth;

    inline bool IsValid() const {
        return !objectId.IsEmpty();
    }

    HDX_API
    size_t GetHash() const;
};

using HdxPickHitVector = std::vector<HdxPickHit>;

/// Information about an instancer instancing a picked object (or an
/// instancer instancing such an instancer and so on).
struct HdxInstancerContext
{
    /// The path of the instancer in the scene index.
    SdfPath instancerSceneIndexPath;
    /// The prim origin data source of the instancer.
    HdContainerDataSourceHandle instancerPrimOrigin;

    /// For implicit instancing (native instancing in USD), the path of
    /// the picked instance in the scene index.
    SdfPath instanceSceneIndexPath;

    /// The prim origin data source of the picked (implicit) instance
    ///
    /// Note that typically, exactly one of instancePrimOrigin
    /// or instancerPrimOrigin will contain data depending on whether
    /// the instancing at the current level was implicit or not,
    /// respectively. This is because for implicit instancing, there is no
    /// authored instancer in the original scene (e.g., no USD instancer
    /// prim for USD native instancing).
    ///
    /// For non-nested implicit instancing, the scenePath of the
    /// instancePrimOrigin will be an absolute path.
    /// For nested implicit instancing, the scenePath of the instancePrimOrigin
    /// is an absolute path for the outer instancer context and a relative
    /// path otherwise.
    /// The relative path corresponds to an instance within a prototype that
    /// was itself instanced. It is relative to the prototype's root.
    ///
    HdContainerDataSourceHandle instancePrimOrigin;
    /// Index of the picked instance.
    int instanceId;
};

/// A helper to extract information about the picked prim that allows
/// modern applications to identify a prim and, e.g., obtain the scene path
/// such as the path of the corresponding UsdPrim.
///
/// Note that this helper assumes that we use scene indices and that the
/// primOrigin data source was populated for each pickable prim in the
/// scene index. Typically, an application will populate the scenePath in the
/// primOrigin data source. But the design allows an application to populate
/// the primOrigin container data source with arbitrary data that helps to
/// give context about a prim and identify the picked prim.
///
/// Note that legacy applications using scene delegates cannot use
/// HdxPrimOriginInfo and have to translate the scene index path to a scene
/// path using the scene delegate API
/// HdSceneDelegate::GetScenePrimPath and
/// HdSceneDelegate::ConvertIndexPathToCachePath.
///
struct HdxPrimOriginInfo
{
    /// Query terminal scene index of render index for information about
    /// picked prim.
    HDX_API
    static HdxPrimOriginInfo
    FromPickHit(HdRenderIndex * renderIndex,
                const HdxPickHit &hit);

    /// Vectorized implementation of function to query terminal scene index
    /// of render index for information about picked prim. Amortizes the cost
    /// of computing the array of all instace indices and locations for
    /// instancers.
    HDX_API
    static std::vector<HdxPrimOriginInfo>
    FromPickHits(HdRenderIndex * renderIndex,
                const std::vector<HdxPickHit> &hits);

    /// Combines instance scene paths and prim scene path to obtain the full
    /// scene path.
    ///
    /// The scene path is extracted from the prim origin container data
    /// source by using the given key.
    ///
    HDX_API
    SdfPath GetFullPath(
        const TfToken &nameInPrimOrigin =
                    HdPrimOriginSchemaTokens->scenePath) const;

    /// Computes an HdInstancerContext object (equivalent to the one computed
    /// by GetScenePrimPath) from the vector of HdxInstancerContext objects.
    /// Note that while HdxInstancerContext has a wealth of data about both
    /// scene instancers and hydra-created instancers, HdInstancerContext
    /// is meant to be just a list of (scene instancer, local instance id)
    /// tuples, used by some clients (e.g. usdview) for selection processing.
    HDX_API
    HdInstancerContext ComputeInstancerContext(
        const TfToken &nameInPrimOrigin =
                    HdPrimOriginSchemaTokens->scenePath) const;

    /// Information about the instancers instancing the picked object.
    /// The outer most instancer will be first.
    std::vector<HdxInstancerContext> instancerContexts;
    /// The prim origin data source for the picked prim if provided
    /// by the scene index.
    HdContainerDataSourceHandle primOrigin;
};

/// Pick task context params.  This contains task params that can't come from
/// the scene delegate (like resolution mode and pick location, that might
/// be resolved late), as well as the picking collection and the output
/// hit vector.
/// 'pickTarget': The target of the pick operation, which may influence the
///     data filled in the HdxPickHit(s).
///     The available options are:
///         HdxPickTokens->pickPrimsAndInstances
///         HdxPickTokens->pickFaces
///         HdxPickTokens->pickEdges
///         HdxPickTokens->pickPoints
///         HdxPickTokens->pickPointsAndInstances
///
/// 'resolveMode': Dictates the resolution of which hit(s) are returned in
///     'outHits'.
///     The available options are:
///     1. HdxPickTokens->resolveNearestToCamera : Returns the hit whose
///         position is nearest to the camera 
///     2. HdxPickTokens->resolveNearestToCenter : Returns the hit whose
///         position is nearest to center of the pick location/region. 
///     3. HdxPickTokens->resolveUnique : Returns the unique hits, by hashing
///         the relevant member fields of HdxPickHit. The 'pickTarget'
///         influences this operation. For e.g., the subprim indices are ignored
///         when the pickTarget is pickPrimsAndInstances.
///     4. HdxPickTokens->resolveAll: Returns all the hits for the pick location
///         or region. The number of hits returned depends on the resolution
///         used and may have duplicates.
///     5. HdxPickTokens->resolveDeep: Returns the unique hits not only of visible 
///         geometry but also of all the geometry hiding behind. The 'pickTarget'
///         influences this operation. For e.g., the subprim indices are ignored
///         when the pickTarget is pickPrimsAndInstances.
///
struct HdxPickTaskContextParams
{
    using DepthMaskCallback = std::function<void(void)>;

    HdxPickTaskContextParams()
        : resolution(128, 128)
        , maxNumDeepEntries(32000)
        , pickTarget(HdxPickTokens->pickPrimsAndInstances)
        , resolveMode(HdxPickTokens->resolveNearestToCamera)
        , doUnpickablesOcclude(false)
        , viewMatrix(1)
        , projectionMatrix(1)
        , clipPlanes()
        , depthMaskCallback(nullptr)
        , collection()
        , alphaThreshold(0.0001f)
        , outHits(nullptr)
    {}

    GfVec2i resolution;
    int maxNumDeepEntries;
    TfToken pickTarget;
    TfToken resolveMode;
    bool doUnpickablesOcclude;
    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;
    std::vector<GfVec4d> clipPlanes;
    DepthMaskCallback depthMaskCallback;
    HdRprimCollection collection;
    float alphaThreshold;
    HdxPickHitVector *outHits;
};

/// \class HdxPickTask
///
/// A task for running picking queries against the current scene.
/// This task generates an id buffer for a "pick frustum" (normally the
/// camera frustum with the near plane narrowed to an (x,y) location and a
/// pick radius); then it resolves that id buffer into a series of prim paths.
/// The "Hit" output also contains subprim picking results (e.g. picked face,
/// edge, point, instance) and the intersection point in scene worldspace.
///
/// HdxPickTask takes an HdxPickTaskParams through the scene delegate, and
/// HdxPickTaskContextParams through the task context as "pickParams".
/// It produces a hit vector, in the task context as "pickHits".
class HdxPickTask : public HdTask
{
public:
    using TaskParams = HdxPickTaskParams;

    HDX_API
    HdxPickTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxPickTask() override;

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;

    /// Prepare the pick task
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute the pick task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

    HDX_API
    const TfTokenVector &GetRenderTags() const override;

    /// Utility: Given a UNorm8Vec4 pixel, unpack it into an int32 ID.
    static inline int DecodeIDRenderColor(unsigned char const idColor[4]) {
        return (int32_t(idColor[0] & 0xff) << 0)  |
               (int32_t(idColor[1] & 0xff) << 8)  |
               (int32_t(idColor[2] & 0xff) << 16) |
               (int32_t(idColor[3] & 0xff) << 24);
    }

private:
    HdxPickTaskParams _params;
    HdxPickTaskContextParams _contextParams;
    TfTokenVector _renderTags;
    bool _useOverlayPass;

    // We need to cache a pointer to the render index so Execute() can
    // map prim ID to paths.
    HdRenderIndex *_index;

    void _InitIfNeeded();
    void _CreateAovBindings();
    void _CleanupAovBindings();
    void _ResizeOrCreateBufferForAOV(
        const HdRenderPassAovBinding& aovBinding);

    void _ConditionStencilWithGLCallback(
            HdxPickTaskContextParams::DepthMaskCallback maskCallback,
            HdRenderBuffer const * depthStencilBuffer);

    bool _UseOcclusionPass() const;
    bool _UseOverlayPass() const;
    void _UpdateUseOverlayPass();

    void _ClearPickBuffer();
    void _ResolveDeep();

    template<typename T>
    HdStTextureUtils::AlignedBuffer<T>
    _ReadAovBuffer(TfToken const & aovName) const;

    HdRenderBuffer const * _FindAovBuffer(TfToken const & aovName) const;

    // Create a shared render pass each for pickables, unpickables, and 
    // items in overlay (which may draw on top even when occluded).
    HdRenderPassSharedPtr _pickableRenderPass;
    HdRenderPassSharedPtr _occluderRenderPass;
    HdRenderPassSharedPtr _overlayRenderPass;

    // Having separate render pass states allows us to use different
    // shader mixins if we choose to (we don't currently).
    HdRenderPassStateSharedPtr _pickableRenderPassState;
    HdRenderPassStateSharedPtr _occluderRenderPassState;
    HdRenderPassStateSharedPtr _overlayRenderPassState;

    Hgi* _hgi;

    std::vector<std::unique_ptr<HdStRenderBuffer>> _pickableAovBuffers;
    HdRenderPassAovBindingVector _pickableAovBindings;
    HdRenderPassAovBinding _occluderAovBinding;
    size_t _pickableDepthIndex;
    TfToken _depthToken;
    std::unique_ptr<HdStRenderBuffer> _overlayDepthStencilBuffer;
    HdRenderPassAovBindingVector _overlayAovBindings;

    // pick buffer used for deep selection
    HdBufferArrayRangeSharedPtr _pickBuffer;

    HdxPickTask() = delete;
    HdxPickTask(const HdxPickTask &) = delete;
    HdxPickTask &operator =(const HdxPickTask &) = delete;
};

/// A utility class for resolving ID buffers into hits.
class HdxPickResult {
public:

    // Pick result takes a tuple of ID buffers:
    // - (primId, instanceId, elementId, edgeId, pointId)
    // along with some geometric buffers:
    // - (depth, Neye)
    // ... and resolves them into a series of hits, using one of the
    // algorithms specified below.
    //
    // index is used to fill in the HdxPickHit structure;
    // pickTarget is used to determine what a valid hit is;
    // viewMatrix, projectionMatrix, depthRange are used for unprojection
    // to calculate the worldSpaceHitPosition and worldSpaceHitNormal.
    // bufferSize is the size of the ID buffers, and subRect is the sub-region
    // of the id buffers to iterate over in the resolution algorithm.
    //
    // All buffers need to be the same size, if passed in.  It's legal for
    // only the depth and primId buffers to be provided; everything else is
    // optional but provides a richer picking result.
    HDX_API
    HdxPickResult(int const* primIds,
                  int const* instanceIds,
                  int const* elementIds,
                  int const* edgeIds,
                  int const* pointIds,
                  int const* neyes,
                  float const* depths,
                  HdRenderIndex const *index,
                  TfToken const& pickTarget,
                  GfMatrix4d const& viewMatrix,
                  GfMatrix4d const& projectionMatrix,
                  GfVec2f const& depthRange,
                  GfVec2i const& bufferSize,
                  GfVec4i const& subRect);

    HDX_API
    ~HdxPickResult();

    HDX_API
    HdxPickResult(HdxPickResult &&);
    HDX_API
    HdxPickResult& operator=(HdxPickResult &&);

    /// Return whether the result was given well-formed parameters.
    HDX_API
    bool IsValid() const;

    /// Return the nearest single hit point. Note that this method may be
    /// considerably more efficient, as it only needs to construct a single
    /// Hit object.
    HDX_API
    void ResolveNearestToCamera(HdxPickHitVector* allHits) const;

    /// Return the nearest single hit point from the center of the viewport.
    /// Note that this method may be considerably more efficient, as it only
    /// needs to construct a single Hit object.
    HDX_API
    void ResolveNearestToCenter(HdxPickHitVector* allHits) const;

    /// Return all hit points. Note that this may contain redundant objects,
    /// however it allows access to all depth values for a given object.
    HDX_API
    void ResolveAll(HdxPickHitVector* allHits) const;

    /// Return the set of unique hit points, keeping only the nearest depth
    /// value.
    HDX_API
    void ResolveUnique(HdxPickHitVector* allHits) const;

private:
    bool _ResolveHit(int index, int x, int y, float z, HdxPickHit* hit) const;

    size_t _GetHash(int index) const;
    bool _IsValidHit(int index) const;

    // Provide accessors for all of the ID buffers.  Since all but _primIds
    // are optional, if the buffer doesn't exist just return -1 (== no hit).
    int _GetPrimId(int index) const {
        return _primIds ? _primIds[index] : -1;
    }
    int _GetInstanceId(int index) const {
        return _instanceIds ? _instanceIds[index] : -1;
    }
    int _GetElementId(int index) const {
        return _elementIds ? _elementIds[index] : -1;
    }
    int _GetEdgeId(int index) const {
        return _edgeIds ? _edgeIds[index] : -1;
    }
    int _GetPointId(int index) const {
        return _pointIds ? _pointIds[index] : -1;
    }

    // Provide an accessor for the normal buffer.  If the normal buffer is
    // provided, this function will unpack the normal.  The fallback is
    // GfVec3f(0.0f).
    GfVec3f _GetNormal(int index) const;

    int const* _primIds;
    int const* _instanceIds;
    int const* _elementIds;
    int const* _edgeIds;
    int const* _pointIds;
    int const* _neyes;
    float const* _depths;
    HdRenderIndex const *_index;
    TfToken  _pickTarget;
    GfMatrix4d _ndcToWorld;
    GfMatrix4d _eyeToWorld;
    GfVec2f _depthRange;
    GfVec2i _bufferSize;
    GfVec4i _subRect;
};

// For sorting, order hits by ndc depth.
HDX_API
bool operator<(HdxPickHit const& lhs, HdxPickHit const& rhs);

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxPickHit& h);
HDX_API
bool operator==(const HdxPickHit& lhs,
                const HdxPickHit& rhs);
HDX_API
bool operator!=(const HdxPickHit& lhs,
                const HdxPickHit& rhs);

HDX_API
std::ostream& operator<<(std::ostream& out, const HdxPickTaskParams& pv);
HDX_API
bool operator==(const HdxPickTaskParams& lhs,
                const HdxPickTaskParams& rhs);
HDX_API
bool operator!=(const HdxPickTaskParams& lhs,
                const HdxPickTaskParams& rhs);

HDX_API
std::ostream& operator<<(std::ostream& out, const HdxPickTaskContextParams& pv);
HDX_API
bool operator==(const HdxPickTaskContextParams& lhs,
                const HdxPickTaskContextParams& rhs);
HDX_API
bool operator!=(const HdxPickTaskContextParams& lhs,
                const HdxPickTaskContextParams& rhs);
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_PICK_TASK_H
