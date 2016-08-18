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

class HdEngine;
class HdRenderIndex;
class HdRprimCollection;

typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderIndex> HdRenderIndexSharedPtr;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfDrawTarget);

HDXLIB_API void HdxNoDepthMask();

class HdxIntersector {
public:
    struct Params;
    class Result;
    struct Hit;

    HDXLIB_API
    HdxIntersector(HdRenderIndexSharedPtr index);
    HDXLIB_API
    ~HdxIntersector() = default;

    /// Given some parameters, populate a collection of resulting hit points,
    /// potentially running commands on the GPU to accelerate the query. Return
    /// false on error or no objects picked.
    ///
    /// Note that the individual hits will still need to be resolved, however no
    /// further GPU execution is required to resolve them.
    HDXLIB_API
    bool Query(HdxIntersector::Params const&,
               HdRprimCollection const&,
               HdEngine*,
               HdxIntersector::Result*);

    /// Set the resolution of the intersector in pixels. Note that setting this
    /// resolution frequently may result in poor preformance.
    HDXLIB_API
    void SetResolution(GfVec2i const& widthHeight);

    enum HitMode {
        HitFirst,
        HitAll
    };

    /// A callback to provide a depth mask. For example, useful for lasso
    /// selection.
    typedef std::function<void(void)> DepthMaskCallback;

    struct Params {
        Params() 
            : hitMode(HitFirst)
            , projectionMatrix(1)
            , viewMatrix(1)
            , alphaThreshold(0.0f)
            , cullStyle(HdCullStyleNothing)
            , depthMaskCallback(HdxNoDepthMask)
        {}

        HitMode hitMode;
        GfMatrix4d projectionMatrix;
        GfMatrix4d viewMatrix;
        float alphaThreshold;
        HdCullStyle cullStyle;
        std::vector<GfVec4d> clipPlanes;
        DepthMaskCallback depthMaskCallback;
    };

    struct Hit {
        SdfPath delegateId;
        SdfPath objectId;
        SdfPath instancerId;
        int instanceIndex;
        int elementIndex;
        GfVec3f worldSpaceHitPoint;
        float ndcDepth;

        inline bool IsValid() const {
            return not objectId.IsEmpty();
        }

        size_t GetHash() const;
        struct Hash {
            inline size_t operator()(Hit const& hit) const {
                return hit.GetHash();
            }
        };

        // Ordered by ndc depth.
        HDXLIB_API bool operator<(Hit const& lhs) const;
        HDXLIB_API bool operator==(Hit const& lhs) const;

        // Depth and position are ignored, used for object/element/instance
        // aggregation.
        struct HitSetHash {
            size_t operator()(Hit const& hit) const;
        };
        // Equality ignores depth and position.
        struct HitSetEq{
            bool operator()(Hit const& a, Hit const& b) const;
        };
    };

    typedef std::vector<Hit> HitVector;
    typedef std::unordered_set<Hit, Hit::HitSetHash, Hit::HitSetEq> HitSet;

    class Result {
    public:
        HDXLIB_API
        Result();
        HDXLIB_API
        Result(std::unique_ptr<unsigned char[]> primIds,
               std::unique_ptr<unsigned char[]> instanceIds,
               std::unique_ptr<unsigned char[]> elementIds,
               std::unique_ptr<float[]> depths,
               HdRenderIndexSharedPtr const& index,
               Params params,
               GfVec4i viewport);
        HDXLIB_API
        ~Result();

        HDXLIB_API
        Result(Result &&);
        HDXLIB_API
        Result& operator=(Result &&);

        inline bool IsValid() const
        {
            return _viewport[2] > 0 and _viewport[3] > 0;
        }

        /// Return the nearest single hit point. Not that this method may be
        /// considerably more efficient, as it only needs to construct a single
        /// Hit object.
        HDXLIB_API
        bool ResolveNearest(HdxIntersector::Hit* hit) const;

        /// Return all hit points. Note that this may contain redundant objects,
        /// however it allows access to all depth values for a given object.
        HDXLIB_API
        bool ResolveAll(HdxIntersector::HitVector* allHits) const;

        /// Return the set of unique hit points, keeping only the nearest depth
        /// value.
        HDXLIB_API
        bool ResolveUnique(HdxIntersector::HitSet* hitSet) const;

    private:
        bool _ResolveHit(int index, int x, int y, float z, Hit* hit) const;
        size_t _GetHash(int index) const;
        
        std::unique_ptr<unsigned char[]> _primIds;
        std::unique_ptr<unsigned char[]> _instanceIds;
        std::unique_ptr<unsigned char[]> _elementIds;
        std::unique_ptr<float[]> _depths;
        HdRenderIndexSharedPtr _index;
        Params _params;
        GfVec4i _viewport;
    };

private:
    void _Init(GfVec2i const&);

    // A single shared render pass is used internally to avoid render pass
    // thrashing due to picking.
    HdRenderPassSharedPtr _renderPass;

    HdRenderPassStateSharedPtr _renderPassState;

    // A single draw target is shared for all contexts. Since the FBO cannot be
    // shared, we clone the attachements on each request.
    GlfDrawTargetRefPtr _drawTarget;

    // The render index for which this intersector is valid.
    HdRenderIndexSharedPtr _index;
};

HDXLIB_API
std::ostream& operator<<(std::ostream& out, HdxIntersector::Hit const & h);

#endif //HDX_INTERSECTOR
