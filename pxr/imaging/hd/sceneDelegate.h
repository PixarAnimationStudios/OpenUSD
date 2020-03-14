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
#ifndef PXR_IMAGING_HD_SCENE_DELEGATE_H
#define PXR_IMAGING_HD_SCENE_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/materialParam.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/textureResource.h"
#include "pxr/imaging/hd/timeSampleArray.h"

#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/hash.h"

#include <boost/shared_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdExtComputationContext;

/// A shared pointer to a vector of id's.
typedef std::shared_ptr<SdfPathVector> HdIdVectorSharedPtr;

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

/// \struct HdDisplayStyle
///
/// Describes how the geometry of a prim should be displayed.
///
struct HdDisplayStyle {
    /// The prim refine level, in the range [0, 8].
    int refineLevel;
    
    /// Is the prim flat shaded.
    bool flatShadingEnabled;
    
    /// Is the prim displacement shaded.
    bool displacementEnabled;
    
    /// Creates a default DisplayStyle.
    /// - refineLevel is 0.
    /// - flatShading is disabled.
    /// - displacement is enabled.
    HdDisplayStyle()
        : refineLevel(0)
        , flatShadingEnabled(false)
        , displacementEnabled(true)
    { }
    
    /// Creates a DisplayStyle.
    /// \param refineLevel_ the refine level to display.
    ///        Valid range is [0, 8].
    /// \param flatShading enables flat shading, defaults to false.
    /// \param displacement enables displacement shading, defaults to false.
    HdDisplayStyle(int refineLevel_,
                   bool flatShading = false,
                   bool displacement = true)
        : refineLevel(std::max(0, refineLevel_))
        , flatShadingEnabled(flatShading)
        , displacementEnabled(displacement)
    {
        if (refineLevel_ < 0) {
            TF_CODING_ERROR("negative refine level is not supported");
        } else if (refineLevel_ > 8) {
            TF_CODING_ERROR("refine level > 8 is not supported");
        }
    }
    
    HdDisplayStyle(HdDisplayStyle const& rhs) = default;
    ~HdDisplayStyle() = default;
    
    bool operator==(HdDisplayStyle const& rhs) const {
        return refineLevel == rhs.refineLevel
            && flatShadingEnabled == rhs.flatShadingEnabled
            && displacementEnabled == rhs.displacementEnabled;
    }
    bool operator!=(HdDisplayStyle const& rhs) const {
        return !(*this == rhs);
    }
};

/// \struct HdPrimvarDescriptor
///
/// Describes a primvar.
struct HdPrimvarDescriptor {
    /// Name of the primvar.
    TfToken name;
    /// Interpolation (data-sampling rate) of the primvar.
    HdInterpolation interpolation;
    /// Optional "role" indicating a desired interpretation --
    /// for example, to distinguish color/vector/point/normal.
    /// See HdPrimvarRoleTokens; default is HdPrimvarRoleTokens->none.
    TfToken role;

    HdPrimvarDescriptor() {}
    HdPrimvarDescriptor(TfToken const& name_,
                        HdInterpolation interp_,
                        TfToken const& role_=HdPrimvarRoleTokens->none)
        : name(name_), interpolation(interp_), role(role_)
    { }
    bool operator==(HdPrimvarDescriptor const& rhs) const {
        return name == rhs.name && role == rhs.role
            && interpolation == rhs.interpolation;
    }
    bool operator!=(HdPrimvarDescriptor const& rhs) const {
        return !(*this == rhs);
    }
};

typedef std::vector<HdPrimvarDescriptor> HdPrimvarDescriptorVector;

/// \struct HdExtComputationPrimvarDescriptor
///
/// Extends HdPrimvarDescriptor to describe a primvar that takes
/// data from the output of an ExtComputation.
///
/// The structure contains the id of the source ExtComputation in the
/// render index, the name of an output from that computation from which
/// the primvar will take data along with a valueType which describes
/// the type of the expected data.
struct HdExtComputationPrimvarDescriptor : public HdPrimvarDescriptor {
    SdfPath sourceComputationId;
    TfToken sourceComputationOutputName;
    HdTupleType valueType;

    HdExtComputationPrimvarDescriptor() {}
    HdExtComputationPrimvarDescriptor(
        TfToken const& name_,
        HdInterpolation interp_,
        TfToken const & role_,
        SdfPath const & sourceComputationId_,
        TfToken const & sourceComputationOutputName_,
        HdTupleType const & valueType_)
        : HdPrimvarDescriptor(name_, interp_, role_)
        , sourceComputationId(sourceComputationId_)
        , sourceComputationOutputName(sourceComputationOutputName_)
        , valueType(valueType_)
    { }
    bool operator==(HdExtComputationPrimvarDescriptor const& rhs) const {
        return HdPrimvarDescriptor::operator==(rhs) &&
            sourceComputationId == rhs.sourceComputationId &&
            sourceComputationOutputName == rhs.sourceComputationOutputName &&
            valueType == rhs.valueType;
    }
    bool operator!=(HdExtComputationPrimvarDescriptor const& rhs) const {
        return !(*this == rhs);
    }
};

typedef std::vector<HdExtComputationPrimvarDescriptor>
        HdExtComputationPrimvarDescriptorVector;

/// \struct HdExtComputationInputDescriptor
///
/// Describes an input to an ExtComputation that takes data from
/// the output of another ExtComputation.
///
/// The structure contains the name of the input and the id of the
/// source ExtComputation in the render index, and which output of
/// that computation to bind the input to.
struct HdExtComputationInputDescriptor {
    TfToken name;
    SdfPath sourceComputationId;
    TfToken sourceComputationOutputName;

    HdExtComputationInputDescriptor() {}
    HdExtComputationInputDescriptor(
        TfToken const & name_,
        SdfPath const & sourceComputationId_,
        TfToken const & sourceComputationOutputName_)
    : name(name_), sourceComputationId(sourceComputationId_)
    , sourceComputationOutputName(sourceComputationOutputName_)
    { }

    bool operator==(HdExtComputationInputDescriptor const& rhs) const {
        return name == rhs.name &&
               sourceComputationId == rhs.sourceComputationId &&
               sourceComputationOutputName == rhs.sourceComputationOutputName;
    }
    bool operator!=(HdExtComputationInputDescriptor const& rhs) const {
        return !(*this == rhs);
    }
};

typedef std::vector<HdExtComputationInputDescriptor>
        HdExtComputationInputDescriptorVector;

/// \struct HdExtComputationOutputDescriptor
///
/// Describes an output of an ExtComputation.
///
/// The structure contains the name of the output along with a valueType
/// which describes the type of the computation output data.
struct HdExtComputationOutputDescriptor {
    TfToken name;
    HdTupleType valueType;

    HdExtComputationOutputDescriptor() {}
    HdExtComputationOutputDescriptor(
        TfToken const & name_,
        HdTupleType const & valueType_)
    : name(name_), valueType(valueType_)
    { }

    bool operator==(HdExtComputationOutputDescriptor const& rhs) const {
        return name == rhs.name &&
               valueType == rhs.valueType;
    }
    bool operator!=(HdExtComputationOutputDescriptor const& rhs) const {
        return !(*this == rhs);
    }
};

typedef std::vector<HdExtComputationOutputDescriptor>
        HdExtComputationOutputDescriptorVector;

/// \struct HdVolumeFieldDescriptor
///
/// Description of a single field related to a volume primitive.
///
struct HdVolumeFieldDescriptor {
    TfToken fieldName;
    TfToken fieldPrimType;
    SdfPath fieldId;

    HdVolumeFieldDescriptor() {}
    HdVolumeFieldDescriptor(
        TfToken const & fieldName_,
        TfToken const & fieldPrimType_,
        SdfPath const & fieldId_)
    : fieldName(fieldName_), fieldPrimType(fieldPrimType_), fieldId(fieldId_)
    { }
};

typedef std::vector<HdVolumeFieldDescriptor>
	HdVolumeFieldDescriptorVector;

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


    /// Gets the axis aligned bounds of a prim.
    /// The returned bounds are in the local space of the prim
    /// (transform is yet to be applied) and should contain the
    /// bounds of any child prims.
    ///
    /// The returned bounds does not include any displacement that
    /// might occur as the result of running shaders on the prim.
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
    virtual HdDisplayStyle GetDisplayStyle(SdfPath const& id);
    
    /// Returns a named value.
    HD_API
    virtual VtValue Get(SdfPath const& id, TfToken const& key);

    /// Returns the authored repr (if any) for the given prim.
    HD_API
    virtual HdReprSelector GetReprSelector(SdfPath const &id);

    /// Returns the render tag that will be used to bucket prims during
    /// render pass bucketing.
    HD_API
    virtual TfToken GetRenderTag(SdfPath const& id);

    /// Returns the prim categories.
    HD_API
    virtual VtArray<TfToken> GetCategories(SdfPath const& id);

    /// Returns the categories for all instances in the instancer.
    HD_API
    virtual std::vector<VtArray<TfToken>>
    GetInstanceCategories(SdfPath const &instancerId);

    /// Returns the coordinate system bindings, or a nullptr if none are bound.
    HD_API
    virtual HdIdVectorSharedPtr GetCoordSysBindings(SdfPath const& id);

    // -----------------------------------------------------------------------//
    /// \name Motion samples
    // -----------------------------------------------------------------------//

    /// Store up to \a maxSampleCount transform samples in \a *sampleValues.
    /// Returns the union of the authored samples and the boundaries 
    /// of the current camera shutter interval. If this number is greater
    /// than maxSampleCount, you might want to call this function again 
    /// to get all the authored data.
    /// Sample times are relative to the scene delegate's current time.
    /// \see GetTransform()
    HD_API
    virtual size_t
    SampleTransform(SdfPath const & id, 
                    size_t maxSampleCount,
                    float *sampleTimes, 
                    GfMatrix4d *sampleValues);

    /// Convenience form of SampleTransform() that takes an HdTimeSampleArray.
    /// This function returns the union of the authored transform samples 
    /// and the boundaries of the current camera shutter interval.
    template <unsigned int CAPACITY>
    void 
    SampleTransform(SdfPath const & id,
                    HdTimeSampleArray<GfMatrix4d, CAPACITY> *sa) {
        size_t authoredSamples = 
            SampleTransform(id, CAPACITY, sa->times.data(), sa->values.data());
        if (authoredSamples > CAPACITY) {
            sa->Resize(authoredSamples);
            size_t authoredSamplesSecondAttempt = 
                SampleTransform(
                    id, 
                    authoredSamples, 
                    sa->times.data(), 
                    sa->values.data());
            // Number of samples should be consisntent through multiple
            // invokations of the sampling function.
            TF_VERIFY(authoredSamples == authoredSamplesSecondAttempt);
        }
        sa->count = authoredSamples;
    }

    /// Store up to \a maxSampleCount transform samples in \a *sampleValues.
    /// Returns the union of the authored samples and the boundaries 
    /// of the current camera shutter interval. If this number is greater
    /// than maxSampleCount, you might want to call this function again 
    /// to get all the authored data.
    /// Sample times are relative to the scene delegate's current time.
    /// \see GetInstancerTransform()
    HD_API
    virtual size_t
    SampleInstancerTransform(SdfPath const &instancerId,
                             size_t maxSampleCount, 
                             float *sampleTimes,
                             GfMatrix4d *sampleValues);

    /// Convenience form of SampleInstancerTransform()
    /// that takes an HdTimeSampleArray.
    /// This function returns the union of the authored samples 
    /// and the boundaries of the current camera shutter interval.
    template <unsigned int CAPACITY>
    void
    SampleInstancerTransform(SdfPath const &instancerId,
                             HdTimeSampleArray<GfMatrix4d, CAPACITY> *sa) {
        size_t authoredSamples = 
            SampleInstancerTransform(
                instancerId, 
                CAPACITY, 
                sa->times.data(), 
                sa->values.data());
        if (authoredSamples > CAPACITY) {
            sa->Resize(authoredSamples);
            size_t authoredSamplesSecondAttempt = 
                SampleInstancerTransform(
                    instancerId, 
                    authoredSamples, 
                    sa->times.data(), 
                    sa->values.data());
            // Number of samples should be consisntent through multiple
            // invokations of the sampling function.
            TF_VERIFY(authoredSamples == authoredSamplesSecondAttempt);
        }
        sa->count = authoredSamples;
    }

    /// Store up to \a maxSampleCount primvar samples in \a *samplesValues.
    /// Returns the union of the authored samples and the boundaries 
    /// of the current camera shutter interval. If this number is greater
    /// than maxSampleCount, you might want to call this function again 
    /// to get all the authored data.
    ///
    /// Sample values that are array-valued will have a size described
    /// by the HdPrimvarDescriptor as applied to the toplogy.
    ///
    /// For example, this means that a mesh that is fracturing over time
    /// will return samples with the same number of points; the number
    /// of points will change as the scene delegate is resynchronzied
    /// to represent the scene at a time with different topology.
    ///
    /// Sample times are relative to the scene delegate's current time.
    ///
    /// \see Get()
    HD_API
    virtual size_t
    SamplePrimvar(SdfPath const& id, 
                  TfToken const& key,
                  size_t maxSampleCount, 
                  float *sampleTimes, 
                  VtValue *sampleValues);

    /// Convenience form of SamplePrimvar() that takes an HdTimeSampleArray.
    /// This function returns the union of the authored samples 
    /// and the boundaries of the current camera shutter interval.
    template <unsigned int CAPACITY>
    void 
    SamplePrimvar(SdfPath const &id, 
                  TfToken const& key,
                  HdTimeSampleArray<VtValue, CAPACITY> *sa) {
        size_t authoredSamples = 
            SamplePrimvar(
                id, 
                key, 
                CAPACITY, 
                sa->times.data(), 
                sa->values.data());
        if (authoredSamples > CAPACITY) {
            sa->Resize(authoredSamples);
            size_t authoredSamplesSecondAttempt = 
                SamplePrimvar(
                    id, 
                    key, 
                    authoredSamples, 
                    sa->times.data(), 
                    sa->values.data());
            // Number of samples should be consisntent through multiple
            // invokations of the sampling function.
            TF_VERIFY(authoredSamples == authoredSamplesSecondAttempt);
        }
        sa->count = authoredSamples;
    }

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
    virtual GfMatrix4d GetInstancerTransform(SdfPath const &instancerId);


    /// Resolves a \p protoRprimId and \p protoIndex back to original
    /// instancer path by backtracking nested instancer hierarchy.
    ///
    /// It will be an empty path if the given protoRprimId is not
    /// instanced.
    ///
    /// (Note: see usdImaging/delegate.h for details specific to USD
    /// instancing)
    ///
    /// If the instancer instances heterogeneously, the instance index of the
    /// prototype rprim doesn't match the instance index in the instancer.
    ///
    /// For example, if we have:
    ///
    ///     instancer {
    ///         prototypes = [ A, B, A, B, B ]
    ///     }
    ///
    /// ...then the indices would be:
    ///
    ///        protoIndex          instancerIndex
    ///     A: [0, 1]              [0, 2]
    ///     B: [0, 1, 2]           [1, 3, 5]
    ///
    /// To track this mapping, the \p instancerIndex is returned which
    /// corresponds to the given protoIndex.
    ///
    /// Also, note if there are nested instancers, the returned path will
    /// be the top-level instancer, and the \p instancerIndex will be for
    /// that top-level instancer. This can lead to situations where, contrary
    /// to the previous scenario, the instancerIndex has fewer possible values
    /// than the \p protoIndex:
    ///
    /// For example, if we have:
    ///
    ///     instancerBottom {
    ///         prototypes = [ A, A, B ]
    ///     }
    ///     instancerTop {
    ///         prototypes = [ instancerBottom, instancerBottom ]
    ///     }
    ///
    /// ...then the indices might be:
    ///
    ///        protoIndex          instancerIndex  (for instancerTop)
    ///     A: [0, 1, 2, 3]        [0, 0, 1, 1]
    ///     B: [0, 1]              [0, 1]
    ///
    /// because \p instancerIndex are indices for instancerTop, which only has
    /// two possible indices.
    ///
    /// If \p masterCachePath is not NULL, then it may be set to the cache path
    /// corresponding instance master prim, if there is one. Otherwise, it will
    /// be set to null.
    ///
    /// If \p instanceContext is not NULL, it is populated with the list of
    /// instance roots that must be traversed to get to the rprim. If this
    /// list is non-empty, the last prim is always the forwarded rprim.
    HD_API
    virtual SdfPath GetPathForInstanceIndex(const SdfPath &protoRprimId,
                                            int protoIndex,
                                            int *instancerIndex,
                                            SdfPath *masterCachePath=NULL,
                                            SdfPathVector *instanceContext=NULL);

    // -----------------------------------------------------------------------//
    /// \name Material Aspects
    // -----------------------------------------------------------------------//
    
    /// Returns the material ID bound to the rprim \p rprimId.
    HD_API
    virtual SdfPath GetMaterialId(SdfPath const &rprimId);

    // Returns a material resource which contains the information 
    // needed to create a material.
    HD_API 
    virtual VtValue GetMaterialResource(SdfPath const &materialId);

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
    /// \name Renderbuffer Aspects
    // -----------------------------------------------------------------------//

    /// Returns the allocation descriptor for a given render buffer prim.
    HD_API
    virtual HdRenderBufferDescriptor GetRenderBufferDescriptor(SdfPath const& id);

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

    /// Returns a single value for a given camera and parameter.
    /// See HdCameraTokens for the list of paramters.
    HD_API
    virtual VtValue GetCameraParamValue(SdfPath const& cameraId,
                                        TfToken const& paramName);

    // -----------------------------------------------------------------------//
    /// \name Volume Aspects
    // -----------------------------------------------------------------------//

    HD_API
    virtual HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(SdfPath const &volumeId);

    // -----------------------------------------------------------------------//
    /// \name ExtComputation Aspects
    // -----------------------------------------------------------------------//

    ///
    /// For the given computation id, returns a list of inputs which
    /// will be requested from the scene delegate using the Get() method.
    ///
    /// See GetExtComputationInputDescriptors and
    /// GetExtComputationOutpuDescriptors for descriptions of other
    /// computation inputs and outputs.
    HD_API
    virtual TfTokenVector
    GetExtComputationSceneInputNames(SdfPath const& computationId);

    ///
    /// For the given computation id, returns a list of computation
    /// input descriptors.
    ///
    /// See HdExtComputationInputDecriptor
    HD_API
    virtual HdExtComputationInputDescriptorVector
    GetExtComputationInputDescriptors(SdfPath const& computationId);

    /// For the given computation id, returns a list of computation
    /// output descriptors.
    ///
    /// See HdExtComputationOutputDescriptor
    HD_API
    virtual HdExtComputationOutputDescriptorVector
    GetExtComputationOutputDescriptors(SdfPath const& computationId);


    /// Returns a list of primvar names that should be bound to
    /// a generated output from  an ExtComputation for the given prim id and
    /// interpolation mode.  Binding information is obtained through
    /// GetExtComputationPrimvarDesc()
    /// Returns a structure describing source information for a primvar
    /// that is bound to an ExtComputation.  See HdExtComputationPrimvarDesc
    /// for the expected information to be returned.
    HD_API
    virtual HdExtComputationPrimvarDescriptorVector
    GetExtComputationPrimvarDescriptors(SdfPath const& id,
                                        HdInterpolation interpolationMode);

    /// Returns a single value for a given computation id and input token.
    /// The token may be a computation input or a computation config parameter.
    HD_API
    virtual VtValue GetExtComputationInput(SdfPath const& computationId,
                                           TfToken const& input);

    /// Returns the kernel source assigned to the computation at the path id.
    /// If the string is empty the computation has no GPU kernel and the
    /// CPU callback should be used.
    HD_API
    virtual std::string GetExtComputationKernel(SdfPath const& computationId);

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

    /// Returns descriptors for all primvars of the given interpolation type.
    HD_API
    virtual HdPrimvarDescriptorVector
    GetPrimvarDescriptors(SdfPath const& id, HdInterpolation interpolation);

    // -----------------------------------------------------------------------//
    /// \name Task Aspects
    // -----------------------------------------------------------------------//
    HD_API
    virtual TfTokenVector GetTaskRenderTags(SdfPath const& taskId);

private:
    HdRenderIndex *_index;
    SdfPath _delegateID;

    HdSceneDelegate() = delete;
    HdSceneDelegate(HdSceneDelegate &) =  delete;
    HdSceneDelegate &operator=(HdSceneDelegate &) =  delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_SCENE_DELEGATE_H
