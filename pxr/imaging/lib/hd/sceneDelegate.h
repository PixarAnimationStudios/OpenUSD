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
#include "pxr/imaging/hd/api.h"
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

class HdExtComputationContext;

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

/// \struct HdExtComputationPrimVarDesc
///
/// Describes a PrimVar that is sourced from an ExtComputation.
/// The scene delegate is expected to fill in this structure for
/// a given primVar name on a specific prim.
///
/// The structure contains the path to the ExtComputation in the render index,
/// and which output on that computation to bind the primVar to.
///
/// The defaultValue provides expected type information about the primVar
/// and may be used in case of error.
struct HdExtComputationPrimVarDesc {
    SdfPath         computationId;
    TfToken         computationOutputName;
    VtValue         defaultValue;
};

/// \struct HdExtComputationInputParams
///
/// Describes a extended information about an input to a ExtComputation.
///
/// In particular, inputs that are bound to the outputs of other
/// ExtComputations.
///
/// The scene delegate is expected to fill in this structure for
/// a given input name on a specific ExtComputation.
///
/// The structure contains the path to the source ExtComputation in the
/// render index, and which output on that computation to bind the input to.
struct HdExtComputationInputParams {
    SdfPath sourceComputationId;
    TfToken computationOutputName;
};

/// \class HdSceneDelegate
///
/// Adapter class providing data exchange with the client scene graph.
///
class HdSceneDelegate {
public:
    /// Constructor used for nested delegate objects which share a RenderIndex.
    HD_API
    HdSceneDelegate(HdRenderIndex *parentIndex,
                    SdfPath const& delegateID);

    HD_API
    virtual ~HdSceneDelegate();

    /// Returns the RenderIndex owned by this delegate.
    HdRenderIndex& GetRenderIndex() { return *_index; }

    /// Returns the ID of this delegate, which is used as a prefix for all
    /// objects it creates in the RenderIndex. 
    ///
    /// The default value is SdfPath::AbsoluteRootPath().
    SdfPath const& GetDelegateID() const { return _delegateID; }

    /// Synchronizes the delegate state for the given request vector.
    HD_API
    virtual void Sync(HdSyncRequestVector* request);

    /// Opportunity for the delegate to clean itself up after
    /// performing parrellel work during sync phase
    HD_API
    virtual void PostSyncCleanup();

    // -----------------------------------------------------------------------//
    /// \name Options
    // -----------------------------------------------------------------------//

    /// Returns true if the named option is enabled by the delegate.
    HD_API
    virtual bool IsEnabled(TfToken const& option) const;


    // -----------------------------------------------------------------------//
    /// \name Rprim Aspects
    // -----------------------------------------------------------------------//

    /// Gets the topological mesh data for a given prim.
    HD_API
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);

    /// Gets the topological curve data for a given prim.
    HD_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);

    /// Gets the subdivision surface tags (sharpness, holes, etc).
    HD_API
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id);

    /// Returns the prim extent in world space (completely untransformed).
    HD_API
    virtual GfRange3d GetExtent(SdfPath const & id);

    /// Returns the object space transform, including all parent transforms.
    HD_API
    virtual GfMatrix4d GetTransform(SdfPath const & id);

    /// Returns the authored visible state of the prim.
    HD_API
    virtual bool GetVisible(SdfPath const & id);

    /// Returns the doubleSided state for the given prim.
    HD_API
    virtual bool GetDoubleSided(SdfPath const & id);

    /// Returns the cullstyle for the given prim.
    HD_API
    virtual HdCullStyle GetCullStyle(SdfPath const &id);

    /// Returns the shading style for the given prim.
    HD_API
    virtual VtValue GetShadingStyle(SdfPath const &id);

    /// Returns the refinement level for the given prim in the range [0,8].
    ///
    /// The refinement level indicates how many iterations to apply when
    /// subdividing subdivision surfaces or other refinable primitives.
    HD_API
    virtual int GetRefineLevel(SdfPath const& id);

    /// Returns a named value.
    HD_API
    virtual VtValue Get(SdfPath const& id, TfToken const& key);

    /// Returns the authored repr (if any) for the given prim.
    HD_API
    virtual TfToken GetReprName(SdfPath const &id);

    /// Returns the render tag that will be used to bucket prims during
    /// render pass bucketing.
    HD_API
    virtual TfToken GetRenderTag(SdfPath const& id, TfToken const& reprName);

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
    HD_API
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerId,
                                          SdfPath const &prototypeId);

    /// Returns the instancer transform.
    HD_API
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
    HD_API
    virtual SdfPath GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                            int instanceIndex,
                                            int *absoluteInstanceIndex,
                                            SdfPath * rprimPath=NULL,
                                            SdfPathVector *instanceContext=NULL);

    // -----------------------------------------------------------------------//
    /// \name SurfaceShader Aspects
    // -----------------------------------------------------------------------//

    /// Returns the source code for the given surface shader ID.
    HD_API
    virtual std::string GetSurfaceShaderSource(SdfPath const &shaderId);

    /// Returns the displacement source code for the given surface shader ID.
    HD_API
    virtual std::string GetDisplacementShaderSource(SdfPath const &shaderId);

    /// Returns the per delegate mixin source code for the given shader stage key.
    /// This allows per-rprim customization of shading for a given delegate.
    /// \see GetShadingStyle
    HD_API
    virtual std::string GetMixinShaderSource(TfToken const &shaderStageKey);

    /// Returns a single value for the given shader and named parameter.
    HD_API
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const &shaderId, 
                                  TfToken const &paramName);

    HD_API
    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const& shaderId);

    // -----------------------------------------------------------------------//
    /// \name Texture Aspects
    // -----------------------------------------------------------------------//

    /// Returns the texture resource ID for a given texture ID.
    HD_API
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);

    /// Returns the texture resource for a given texture ID.
    HD_API
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);

    // -----------------------------------------------------------------------//
    /// \name Light Aspects
    // -----------------------------------------------------------------------//

    // Returns a single value for a given light and parameter.
    HD_API
    virtual VtValue GetLightParamValue(SdfPath const &id, 
                                       TfToken const &paramName);

    // -----------------------------------------------------------------------//
    /// \name Camera Aspects
    // -----------------------------------------------------------------------//

    /// Returns an array of clip plane equations in eye-space with y-up
    /// orientation.
    HD_API
    virtual std::vector<GfVec4d> GetClipPlanes(SdfPath const& cameraId);

    // -----------------------------------------------------------------------//
    /// \name ExtComputation Aspects
    // -----------------------------------------------------------------------//

    ///
    /// For the given computation id and input type, returns a list of
    /// name tokens.
    ///
    /// If the input type is scene, the input is requested from the scene
    /// delegate using the Get() method.
    ///
    /// If the input type is computation, the input is bound to another
    /// ExtComputation.  GetExtComputationInputParams() is used to obtain
    /// the binding information for the input.
    ///
    HD_API
    virtual TfTokenVector GetExtComputationInputNames(SdfPath const& id,
                                                HdExtComputationInputType type);

    /// Obtain extended information about an input to an ExtComputation,
    /// such as binding information.
    ///
    /// The ExtComputation is identified by id, with the specific input
    /// identified by input name.
    ///
    /// See HdExtComputationInputParams for the information the scene delegate
    /// is expected to provide.
    HD_API
    virtual HdExtComputationInputParams GetExtComputationInputParams(
                                   SdfPath const& id, TfToken const &inputName);

    HD_API
    virtual TfTokenVector GetExtComputationOutputNames(SdfPath const& id);

    /// Requests the scene delegate run the ExtComputation with the given id.
    /// The context contains the input values that delegate requested through
    /// GetExtComputationInputNames().
    ///
    /// The scene delegate is expected to set each output identified by
    /// GetExtComputationOutputNames() on the context.
    ///
    /// Hydra may invoke the computation on a different thread from
    /// what HdEngine::Execute() was called on.  It may also invoke
    /// many computations in parallel.
    HD_API
    virtual void InvokeExtComputation(SdfPath const& computationId,
                                      HdExtComputationContext *context);



    // -----------------------------------------------------------------------//
    /// \name Primitive Variables
    // -----------------------------------------------------------------------//

    /// Returns the vertex-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);

    /// Returns the varying-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id);

    /// Returns the Facevarying-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id);

    /// Returns the Uniform-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id);

    /// Returns the Constant-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);

    /// Returns the Instance-rate primVar names.
    HD_API
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id);

    /// Returns a list of primVar names that should be bound to
    /// a generated output from  anExtComputation for the given prim id and
    /// interpolation mode.  Binding information is obtained through
    /// GetExtComputationPrimVarDesc()
    HD_API
    virtual TfTokenVector GetExtComputationPrimVarNames(
                                             SdfPath const& id,
                                             HdInterpolation interpolationMode);

    /// Returns a structure describing source information for a primVar
    /// that is bound to an ExtComputation.  See HdExtComputationPrimVarDesc
    /// for the expected information to be returned.
    HD_API
    virtual HdExtComputationPrimVarDesc GetExtComputationPrimVarDesc(
                                                SdfPath const& id,
                                                TfToken const& varName);
    
    /// Returns the kernel source assigned to the computation at the path id.
    /// If the string is empty the computation has no GPU kernel and the
    /// CPU callback should be used.
    HD_API
    virtual std::string GetExtComputationKernel(SdfPath const& id);

private:
    HdRenderIndex *_index;
    SdfPath _delegateID;

    HdSceneDelegate() = delete;
    HdSceneDelegate(HdSceneDelegate &) =  delete;
    HdSceneDelegate &operator=(HdSceneDelegate &) =  delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SCENE_DELEGATE_H
