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
#ifndef HD_SCENE_DELEGATE_H
#define HD_SCENE_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/shaderParam.h"
#include "pxr/imaging/hd/textureResource.h"

#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/hash.h"

#include <boost/shared_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdRenderIndex> HdRenderIndexSharedPtr;

/// \class HdSyncRequestVector
///
/// The SceneDelegate is requested to synchronize prims as the result of
/// executing a specific render pass, the following data structure is passed
/// back to the delegate to drive synchronization.
///
struct HdSyncRequestVector {
    // The Prims to synchronize in this request.
    SdfPathVector IDs;

    // The HdChangeTracker::DirtyBits that are set for each Prim.
    std::vector<HdDirtyBits> dirtyBits;
};


/// \class HdSceneDelegate
///
/// Adapter class providing data exchange with the client scene graph.
///
class HdSceneDelegate {
public:

    /// Default constructor initializes its own RenderIndex.
    HdSceneDelegate();
    
    /// Constructor used for nested delegate objects which share a RenderIndex.
    HdSceneDelegate(HdRenderIndexSharedPtr const& parentIndex, 
                    SdfPath const& delegateID);

    virtual ~HdSceneDelegate();

    /// Returns the RenderIndex owned by this delegate.
    HdRenderIndex& GetRenderIndex() { return *_index; }

    /// Returns the ID of this delegate, which is used as a prefix for all
    /// objects it creates in the RenderIndex. 
    ///
    /// The default value is SdfPath::AbsoluteRootPath().
    SdfPath const& GetDelegateID() const { return _delegateID; }

    /// Synchronizes the delegate state for the given request vector.
    virtual void Sync(HdSyncRequestVector* request);

    /// Opportunity for the delegate to clean itself up after
    /// performing parrellel work during sync phase
    virtual void PostSyncCleanup();

    // -----------------------------------------------------------------------//
    /// \name Options
    // -----------------------------------------------------------------------//

    /// Returns true if the named option is enabled by the delegate.
    virtual bool IsEnabled(TfToken const& option) const;

    // -----------------------------------------------------------------------//
    /// \name Collections
    // -----------------------------------------------------------------------//

    /// Returns true if the prim identified by \p id is in the named collection.
    virtual bool IsInCollection(SdfPath const& id, 
                                TfToken const& collectionName);

    // -----------------------------------------------------------------------//
    /// \name Rprim Aspects
    // -----------------------------------------------------------------------//

    /// Gets the topological mesh data for a given prim.
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);

    /// Gets the topological curve data for a given prim.
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);

    /// Gets the subdivision surface tags (sharpness, holes, etc).
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id);

    /// Returns the prim extent in world space (completely untransformed).
    virtual GfRange3d GetExtent(SdfPath const & id);

    /// Returns the object space transform, including all parent transforms.
    virtual GfMatrix4d GetTransform(SdfPath const & id);

    /// Returns the authored visible state of the prim.
    virtual bool GetVisible(SdfPath const & id);

    /// Returns the doubleSided state for the given prim.
    virtual bool GetDoubleSided(SdfPath const & id);

    /// Returns the cullstyle for the given prim.
    virtual HdCullStyle GetCullStyle(SdfPath const &id);

    /// Returns the refinement level for the given prim in the range [0,8].
    ///
    /// The refinement level indicates how many iterations to apply when
    /// subdividing subdivision surfaces or other refinable primitives.
    virtual int GetRefineLevel(SdfPath const& id);

    /// Returns a named value.
    virtual VtValue Get(SdfPath const& id, TfToken const& key);

    /// Returns the authored repr (if any) for the given prim.
    virtual TfToken GetReprName(SdfPath const &id);

    /// Returns the render tag that will be used to bucked prims during
    /// render pass bucketing.
    virtual TfToken GetRenderTag(SdfPath const& id);

    // -----------------------------------------------------------------------//
    /// \name Instancer prototypes
    // -----------------------------------------------------------------------//

    /// Gets the extracted indices array of the prototype id used in the
    /// instancer.
    ///
    /// example
    ///  instances:  0, 1, 2, 3, 4, 5
    ///  protoypes:  A, B, A, A, B, C
    ///
    ///    GetInstanceIndices(A) : [0, 2, 3]
    ///    GetInstanceIndices(B) : [1, 4]
    ///    GetInstanceIndices(C) : [5]
    ///    GetInstanceIndices(D) : []
    ///
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerId,
                                          SdfPath const &prototypeId);

    /// Returns the instancer transform.
    virtual GfMatrix4d GetInstancerTransform(SdfPath const &instancerId,
                                             SdfPath const &prototypeId);


    /// Resolves a pair of rprimPath and instanceIndex back to original
    /// (instance) path by backtracking nested instancer hierarchy.
    ///
    /// if the instancer instances heterogeneously, instanceIndex of the
    /// prototype rprim doesn't match the instanceIndex in the instancer.
    ///
    /// for example:
    ///   instancer = [ A, B, A, B, B ]
    ///        instanceIndex       absoluteInstanceIndex
    ///     A: [0, 1]              [0, 2]
    ///     B: [0, 1, 2]           [1, 3, 5]
    ///
    /// To track this mapping, absoluteInstanceIndex is returned which
    /// is an instanceIndex of the instancer for the given instanceIndex of
    /// the prototype.
    ///
    virtual SdfPath GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                            int instanceIndex,
                                            int *absoluteInstanceIndex,
                                            SdfPath * rprimPath=NULL,
                                            SdfPathVector *instanceContext=NULL);

    // -----------------------------------------------------------------------//
    /// \name SurfaceShader Aspects
    // -----------------------------------------------------------------------//

    /// Returns the source code for the given surface shader ID.
    virtual std::string GetSurfaceShaderSource(SdfPath const &shaderId);

    /// Returns the displacement source code for the given surface shader ID.
    virtual std::string GetDisplacementShaderSource(SdfPath const &shaderId);

    /// Returns a vector of shader parameter names. These names can be used to
    /// fetch parameter values.
    virtual TfTokenVector GetSurfaceShaderParamNames(SdfPath const &shaderId);

    /// Returns a single value for the given shader and named parameter.
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const &shaderId, 
                                  TfToken const &paramName);

    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const& shaderId);

    /// Returns a vector of texture IDs for the given surface shader ID.
    virtual SdfPathVector GetSurfaceShaderTextures(SdfPath const &shaderId);

    // -----------------------------------------------------------------------//
    /// \name Texture Aspects
    // -----------------------------------------------------------------------//

    /// Returns the texture resource ID for a given texture ID.
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);

    /// Returns the texture resource for a given texture ID.
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);

    // -----------------------------------------------------------------------//
    /// \name Camera Aspects
    // -----------------------------------------------------------------------//

    /// Returns an array of clip plane equations in eye-space with y-up
    /// orientation.
    virtual std::vector<GfVec4d> GetClipPlanes(SdfPath const& cameraId);

    // -----------------------------------------------------------------------//
    /// \name Primitive Variables
    // -----------------------------------------------------------------------//

    /// Returns the vertex-rate primVar names.
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);

    /// Returns the varying-rate primVar names.
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id);

    /// Returns the Facevarying-rate primVar names.
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id);

    /// Returns the Uniform-rate primVar names.
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id);

    /// Returns the Constant-rate primVar names.
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);

    /// Returns the Instance-rate primVar names.
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id);

    /// Returns the primVar data type.
    virtual int GetPrimVarDataType(SdfPath const& id, TfToken const& key);

    /// Returns the number of components in the primVar, for example a
    /// vec4-valued primVar would return 4.
    virtual int GetPrimVarComponents(SdfPath const& id, TfToken const& key);

private:
    HdRenderIndexSharedPtr _index;
    SdfPath _delegateID;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SCENE_DELEGATE_H
