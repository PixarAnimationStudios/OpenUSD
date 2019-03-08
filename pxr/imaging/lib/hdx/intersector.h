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
#ifndef HDX_INTERSECTOR
#define HDX_INTERSECTOR

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/usd/sdf/path.h"

#include <boost/shared_ptr.hpp>

#include <functional>
#include <memory>
#include <vector>
#include <unordered_set>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


class HdEngine;
class HdRenderIndex;
class HdRprimCollection;

typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassState> 
    HdStRenderPassStateSharedPtr;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfDrawTarget);

class HdxIntersector {
public:
    struct Params;
    class Result;
    struct Hit;

    HDX_API
    HdxIntersector(HdRenderIndex *index);
    HDX_API
    ~HdxIntersector() = default;

    /// The ID render pass encodes the ID as color in a specific order.
    /// Use this method to ensure the read back is done in an endian
    /// correct fashion.
    ///
    /// As packing of IDs may change in the future we encapsulate the
    /// correct behavior here.
    /// \param idColor a byte buffer of length 4.
    static inline int DecodeIDRenderColor(unsigned char const idColor[4]) {
        return (int32_t(idColor[0] & 0xff) << 0)  |
               (int32_t(idColor[1] & 0xff) << 8)  |
               (int32_t(idColor[2] & 0xff) << 16) |
               (int32_t(idColor[3] & 0xff) << 24);
    }

    /// Given some parameters, populate a collection of resulting hit points,
    /// potentially running commands on the GPU to accelerate the query. Return
    /// false on error or no objects picked.
    ///
    /// Note that the individual hits will still need to be resolved, however no
    /// further GPU execution is required to resolve them.
    HDX_API
    bool Query(HdxIntersector::Params const&,
               HdRprimCollection const&,
               HdEngine*,
               HdxIntersector::Result*);

    /// Set the resolution of the intersector in pixels. Note that setting this
    /// resolution frequently may result in poor performance.
    HDX_API
    void SetResolution(GfVec2i const& widthHeight);

    enum HitMode {
        HitFirst,
        HitAll
    };

    /// A callback to provide a depth mask. For example, useful for lasso
    /// selection.
    typedef std::function<void(void)> DepthMaskCallback;

    /// The target of the picking operation, which allows us to write out the
    /// minimal number of id's during the ID render pass.
    enum PickTarget {
        PickPrimsAndInstances = 0,
        PickFaces,
        PickEdges,
        PickPoints
    };

    struct Params {
        Params() 
            : hitMode(HitFirst)
            , pickTarget(PickPrimsAndInstances)
            , doUnpickablesOcclude(false)
            , projectionMatrix(1)
            , viewMatrix(1)
            , alphaThreshold(0.0f)
            , cullStyle(HdCullStyleNothing)
            , depthMaskCallback(nullptr)
            , renderTags()
            , enableSceneMaterials(true)
        {}

        HitMode hitMode;
        PickTarget pickTarget;
        bool doUnpickablesOcclude;
        GfMatrix4d projectionMatrix;
        GfMatrix4d viewMatrix;
        float alphaThreshold;
        HdCullStyle cullStyle;
        std::vector<GfVec4d> clipPlanes;
        DepthMaskCallback depthMaskCallback;
        TfTokenVector renderTags;
        bool enableSceneMaterials;
    };

    struct Hit {
        SdfPath delegateId;
        SdfPath objectId;
        SdfPath instancerId;
        int instanceIndex;
        int elementIndex;
        int edgeIndex;
        int pointIndex;
        GfVec3f worldSpaceHitPoint;
        float ndcDepth;

        inline bool IsValid() const {
            return !objectId.IsEmpty();
        }

        HDX_API
        size_t GetHash() const;
        struct Hash {
            inline size_t operator()(Hit const& hit) const {
                return hit.GetHash();
            }
        };

        // Ordered by ndc depth.
        HDX_API
        bool operator<(Hit const& lhs) const;
        HDX_API
        bool operator==(Hit const& lhs) const;

        // Depth and position are ignored, used for object/instance/subprimitive
        // aggregation.
        struct HitSetHash {
            HDX_API
            size_t operator()(Hit const& hit) const;
        };
        // Equality ignores depth and position.
        struct HitSetEq{
            HDX_API
            bool operator()(Hit const& a, Hit const& b) const;
        };
    };

    typedef std::vector<Hit> HitVector;
    typedef std::unordered_set<Hit, Hit::HitSetHash, Hit::HitSetEq> HitSet;

    class Result {
    public:
        HDX_API
        Result();
        HDX_API
        Result(std::unique_ptr<unsigned char[]> primIds,
               std::unique_ptr<unsigned char[]> instanceIds,
               std::unique_ptr<unsigned char[]> elementIds,
               std::unique_ptr<unsigned char[]> edgeIds,
               std::unique_ptr<unsigned char[]> pointIds,
               std::unique_ptr<float[]> depths,
               HdRenderIndex const *index,
               Params params,
               GfVec4i viewport);
        HDX_API
        ~Result();

        HDX_API
        Result(Result &&);
        HDX_API
        Result& operator=(Result &&);

        inline bool IsValid() const
        {
            return _viewport[2] > 0 && _viewport[3] > 0;
        }

        /// Return the nearest single hit point. Note that this method may be
        /// considerably more efficient, as it only needs to construct a single
        /// Hit object.
        HDX_API
        bool ResolveNearestToCamera(HdxIntersector::Hit* hit) const;

        /// Return the nearest single hit point from the center of the viewport.
        /// Note that this method may be considerably more efficient, as it only
        /// needs to construct a single Hit object.
        HDX_API
        bool ResolveNearestToCenter(HdxIntersector::Hit* hit) const;

        /// Return all hit points. Note that this may contain redundant objects,
        /// however it allows access to all depth values for a given object.
        HDX_API
        bool ResolveAll(HdxIntersector::HitVector* allHits) const;

        /// Return the set of unique hit points, keeping only the nearest depth
        /// value.
        HDX_API
        bool ResolveUnique(HdxIntersector::HitSet* hitSet) const;

    private:
        bool _ResolveHit(int index, int x, int y, float z, Hit* hit) const;
        size_t _GetHash(int index) const;
        bool _IsIdValid(unsigned char const* ids, int index) const;
        bool _IsPrimIdValid(int index) const;
        bool _IsPointIdValid(int index) const;
        bool _IsEdgeIdValid(int index) const;
        bool _IsValidHit(int index) const;
        
        std::unique_ptr<unsigned char[]> _primIds;
        std::unique_ptr<unsigned char[]> _instanceIds;
        std::unique_ptr<unsigned char[]> _elementIds;
        std::unique_ptr<unsigned char[]> _edgeIds;
        std::unique_ptr<unsigned char[]> _pointIds;
        std::unique_ptr<float[]> _depths;
        HdRenderIndex const *_index;
        Params _params;
        GfVec4i _viewport;
    };

private:
    void _Init(GfVec2i const&);
    void _ConditionStencilWithGLCallback(DepthMaskCallback callback);
    void _ConfigureSceneMaterials(bool enableSceneMaterials,
        HdStRenderPassState *renderPassState);

    // Create a shared render pass each for pickables and unpickables
    HdRenderPassSharedPtr _pickableRenderPass;
    HdRenderPassSharedPtr _occluderRenderPass;

    // Override shader is used when scene materials are disabled
    HdStShaderCodeSharedPtr _overrideShader;

    // Having separate render pass states allows us to queue up the tasks
    // corresponding to each of the above render passes. It also lets us use
    // different shader mixins if we choose to (we don't currently.)
    HdRenderPassStateSharedPtr _pickableRenderPassState;
    HdRenderPassStateSharedPtr _occluderRenderPassState;

    // A single draw target is shared for all contexts. Since the FBO cannot be
    // shared, we clone the attachements on each request.
    GlfDrawTargetRefPtr _drawTarget;

    // The render index for which this intersector is valid.
    HdRenderIndex *_index;
};

HDX_API
std::ostream& operator<<(std::ostream& out, HdxIntersector::Hit const & h);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_INTERSECTOR
