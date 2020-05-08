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
#include "pxr/usd/usdSkel/bakeSkinning.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/binding.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/blendShapeQuery.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/debugCodes.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"
#include "pxr/usd/usdSkel/utils.h"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <unordered_map>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((xformOpTransform, "xformOp:transform"))
    (Xform)
);


/*

  Skinning Overview:

  UsdSkel skinning is not a terribly complex operation, and can be summarized
  with pseudo code as:

  for frame in frames:
     for skel in skels:
         skinningXforms = ComputeSkinningTransforms(skel, frame)
         blendShapeWeights = ComputeBlendShapeWeights(skel, frame)
         
         for prim in primsSkinningBySkel(skel):
             DeformPrimWithBlendShapes(prim, blendShapeWeights)
             DeformPrimWithLBS(prim, skinningXforms)
             WriteResults(prim)

   However, doing this *efficiently* requires a few additional considerations:

   - Values read from disk should be read only once, then cached.
     For example, the rest points that we deform should probably only
     be read once.

   - Different skels may be authored at different time samples.
     Skels should only be processed on the time samples that matter.

   - Should only compute data where needed. For instance, if the skinnable prims
     associated with a skel do not define blend shapes, then there's no point
     in wasting cycles reading in blend shape weight animation.

   - For efficient authoring, writes should happen in Sdf, and change
     processing should be deferred so that it can be batched.

   - Writes to Sdf are retained in-memory until Save() is called.
     If a large crowd is being baked, it's possible that there is insufficient
     memory to hold all posed points. To work around this, we need to keep
     track of memory usage (or at least an estimate), so that pending writes may
     be periodically flushed to disk with SdfLayer::Save()).

   The considerations above account for most of the code that follows.

*/


namespace {


std::string
_DeformationFlagsToString(int flags, const char* indent)
{
    return TfStringPrintf(
        "%sdeformPointsWithLBS: %d\n"
        "%sdeformNormalsWithLBS: %d\n"
        "%sdeformXformWithLBS: %d\n"
        "%sdeformPointsWithBlendShapes: %d\n"
        "%sdeformNormalsWithBlendShapes: %d\n",
        indent, bool(flags&UsdSkelBakeSkinningParms::DeformPointsWithLBS),
        indent, bool(flags&UsdSkelBakeSkinningParms::DeformNormalsWithLBS),
        indent, bool(flags&UsdSkelBakeSkinningParms::DeformXformWithLBS),
        indent, bool(flags&UsdSkelBakeSkinningParms::
                     DeformPointsWithBlendShapes),
        indent, bool(flags&UsdSkelBakeSkinningParms::
                     DeformNormalsWithBlendShapes));
}


// ------------------------------------------------------------
// _Task
// ------------------------------------------------------------


/// Helper for managing exec of a task over time.
/// This class only manages the state of the computation; The actual computation
/// and its results are maintained externally from this class.
struct _Task
{
    _Task() {
        Clear();
    }

    void Clear() {
        _active = false;
        _required = false;
        _mightBeTimeVarying = false;
        _isFirstSample = true;
        _hasSampleAtCurrentTime = false;
    }

    explicit operator bool() const {
        return _active && _required;
    }

    /// Returns true if a computation is active.
    /// An active computation does not necessarily need to run.
    bool IsActive() const {
        return _active;
    }

    /// Run \p fn at \p time, if necessary.
    template <class Fn>
    bool Run(const UsdTimeCode time,
             const UsdPrim& prim,
             const char* name,
             const Fn& fn) {

        if (!*this) {
            return false;
        }

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]     Try to run task '%s' for <%s>.\n",
            name, prim.GetPath().GetText());

        // Always compute for defaults.
        // For numeric times, if the task might be time varying, the task
        // is always computed. Otherwise, it is only computed the
        /// first time through.
        if (_mightBeTimeVarying || _isFirstSample || time.IsDefault()) {
                
            _hasSampleAtCurrentTime = fn(time);

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]       Ran task '%s' for <%s>. "
                "Result: %d\n", name, prim.GetPath().GetText(),
                _hasSampleAtCurrentTime);

            if (time.IsNumeric()) {
                _isFirstSample = false;
            }
        } else {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]       Skipping task '%s' "
                "for <%s>. Unvarying task has already been computed.\n",
                name, prim.GetPath().GetText());
        }
        return _hasSampleAtCurrentTime;
    }

    /// Returns true if the task was successfully processed to update
    /// some cached value.
    /// The actual cached value is held externally.
    bool HasSampleAtCurrentTime() const {
        return _hasSampleAtCurrentTime;
    }

    /// Set a flag indicating that the computation is needed by something.
    void SetRequired(bool required) {
        _required = required;
    }

    /// Set the active status of the computation.
    /// The active status indicates whether or not a computation can be run.
    void SetActive(bool active, bool required=true) {
        _active = active;
        _required = required;
    }

    /// Returns true if the result of this task might vary over time.
    bool MightBeTimeVarying() const {
        return _mightBeTimeVarying;
    }

    /// Set a flag indicating whether or not the result of a computation
    /// *might* vary over time.
    void SetMightBeTimeVarying(bool tf) {
        _mightBeTimeVarying = tf;
    }

    std::string GetDescription() const {
        return TfStringPrintf(
            "active: %d, required: %d, mightBeTimeVarying: %d",
            _active, _required, _mightBeTimeVarying);
    }

private:
    bool _active : 1;
    bool _required : 1;
    bool _mightBeTimeVarying : 1;
    bool _isFirstSample : 1;
    bool _hasSampleAtCurrentTime : 1;
};


// ------------------------------------------------------------
// _OutputHolder
// ------------------------------------------------------------


/// Helper for holding a pending output value.
template <class T>
struct _OutputHolder
{
    void BeginUpdate() {
        hasSampleAtCurrentTime = false;
    }

    T value;
    bool hasSampleAtCurrentTime;
};


// ------------------------------------------------------------
// _AttrWriter
// ------------------------------------------------------------


/// Helper for efficiently writing attribute values through Sdf.
struct _AttrWriter
{
    explicit operator bool() const { return bool(_spec); }

    /// Define an attribute spec on \p prim.
    bool Define(const SdfPrimSpecHandle& prim,
                const TfToken& name,
                const SdfValueTypeName& typeName,
                SdfVariability variability);

    /// Write the value to the Sdf layer.
    /// Returns an estimate of the number of bytes consumed by the data 
    /// that was written.
    template <typename T>
    size_t Set(const T& value, UsdTimeCode time);
                
private:
    SdfAttributeSpecHandle _spec;
    SdfPath _primPath;
    TfToken _name;
};


const SdfAttributeSpecHandle
_CreateAttribute(const SdfPrimSpecHandle& owner,
                 const TfToken& name,
                 const SdfValueTypeName& typeName,
                 SdfVariability variability)
{
    const auto& attrView = owner->GetAttributes();

    const auto it = attrView.find(name);
    if (it == attrView.end()) {
        return SdfAttributeSpec::New(owner, name, typeName,
                                     variability, /*custom*/ false);
    } else {
        const auto& spec = *it;
        // Make sure the type matches...
        if (spec->GetTypeName() == typeName) {
            return spec;
        } else {
            TF_RUNTIME_ERROR("Spec type mismatch. Failed to create attribute "
                             "for <%s.%s> in @%s@. Spec with type %s already " 
                            "at that location.",
                             owner->GetPath().GetText(), name.GetText(),
                             owner->GetLayer()->GetIdentifier().c_str(),
                             TfStringify(spec->GetTypeName()).c_str());
        }
    }
    return TfNullPtr;
}


bool
_AttrWriter::Define(const SdfPrimSpecHandle& prim,
                    const TfToken& name,
                    const SdfValueTypeName& typeName,
                    SdfVariability variability)
{
    if ((_spec = _CreateAttribute(prim, name, typeName, variability))) {
        // Clear any prior animation.
        _spec->ClearInfo(SdfFieldKeys->TimeSamples);

        _primPath = prim->GetPath();
        _name = name;
        return true;
    }
    return false;
}


template <class T>
size_t
_GetSizeEstimate(const VtArray<T>& value)
{
    return value.size()*sizeof(T) + sizeof(VtArray<T>);
}


template <class T>
size_t
_GetSizeEstimate(const T& value)
{
    return sizeof(T);
}


template <class T>
size_t
_AttrWriter::Set(const T& value, UsdTimeCode time)
{
    TF_VERIFY(_spec);

    if (time.IsNumeric()) {
        const SdfPath path = _primPath.AppendProperty(_name);
        _spec->GetLayer()->SetTimeSample(path, time.GetValue(), value);
    } else {
        _spec->SetDefaultValue(VtValue(value));
    }
    return _GetSizeEstimate(value);
}


// ------------------------------------------------------------
// _SkelAdapter
// ------------------------------------------------------------


/// Object which interfaces with USD to pull on skel animation data,
/// and cache data where appropriate.
/// This augments a UsdSkelSkeletonQuery to perform additional caching
/// based on variability.
///
/// The execution procedure for a skel adapter may be summarized as:
/// \code
///     UsdGeomXformCache xfCache;
///     for (i,time : times) {
///         xfCache.SetTime(time);
///         skelAdapter.UpdateTransform(i, &xfCache);
///         skelAdapter.UpdateAnimation(time);
///         ...
///         // Apply skinning.
///     }
/// \endcode
///
/// The per-frame update is split into separate calls for the sake of threading:
/// UsdGeomXformCache is not thread-safe, and so the update step that uses an
/// xform cache must be done in serial, whereas UpdateAnimation() may be safely
/// called on different skel adapters in parallel.
struct _SkelAdapter
{
   _SkelAdapter(const UsdSkelBakeSkinningParms& parms,
                const UsdSkelSkeletonQuery& skelQuery,
                UsdGeomXformCache* xfCache);

    UsdPrim GetPrim() const {
        return _skelQuery.GetPrim();
    }

    bool ShouldProcessAtTime(const size_t timeIndex) const {
        TF_DEV_AXIOM(timeIndex < _timeSampleMask.size());
        return _timeSampleMask[timeIndex];
    }
    
    /// Append additional time samples of the skel to \p times.
    void ExtendTimeSamples(const GfInterval& interval,
                           std::vector<double>* times);

    /// Use \p xfCache to update any transforms required for skinning.
    void UpdateTransform(const size_t timeIndex, UsdGeomXformCache* xfCache);

    /// Update any animation data needed for skinning.
    void UpdateAnimation(const UsdTimeCode time, const size_t timeIndex);

    bool GetSkinningTransforms(VtMatrix4dArray* xforms) const {
        if (_skinningXformsTask.HasSampleAtCurrentTime()) {
            *xforms = _skinningXforms;
            return true;
        }
        return false;
    }

    bool GetSkinningInvTransposeTransforms(VtMatrix3dArray* xforms) const {
        if (_skinningInvTransposeXformsTask.HasSampleAtCurrentTime()) {
            *xforms = _skinningInvTransposeXforms;
            return true;
        }
        return false;
    }

    bool GetBlendShapeWeights(VtFloatArray* weights) const {
        if (_blendShapeWeightsTask.HasSampleAtCurrentTime()) {
            *weights = _blendShapeWeights;
            return true;
        }
        return false;
    }

    bool GetLocalToWorldTransform(GfMatrix4d* xf) const {
        if (_skelLocalToWorldXformTask.HasSampleAtCurrentTime()) {
            *xf = _skelLocalToWorldXform;
            return true;
        }
        return false;
    }

    void SetTimeSampleMask(std::vector<bool>&& mask) {  
        _timeSampleMask = std::move(mask);
    }

    bool CanComputeSkinningXforms() const {
        return _skinningXformsTask.IsActive();
    }

    void SetSkinningXformsRequired(bool required) {
        _skinningXformsTask.SetRequired(required);
    }

    bool CanComputeSkinningInvTransposeXforms() const {
        return _skinningInvTransposeXformsTask.IsActive();
    }

    void SetSkinningInvTransposeXformsRequired(bool required) {
        _skinningInvTransposeXformsTask.SetRequired(required);
    }

    bool CanComputeBlendShapeWeights() const {
        return _blendShapeWeightsTask.IsActive();
    }

    void SetBlendShapeWeightsRequired(bool required) {
        _blendShapeWeightsTask.SetRequired(required);
    }

    void SetLocalToWorldXformRequired(bool required) {
        _skelLocalToWorldXformTask.SetRequired(required);
    }

    bool HasTasksToRun() const {
        return _skinningXformsTask ||
               _skinningInvTransposeXformsTask ||
               _blendShapeWeightsTask ||
               _skelLocalToWorldXformTask;
    }

private:

    void _ComputeSkinningXforms(const UsdTimeCode time);

    void _ComputeSkinningInvTransposeXforms(const UsdTimeCode time);

    void _ComputeBlendShapeWeights(const UsdTimeCode time);

private:
    UsdSkelSkeletonQuery _skelQuery;

    /// Skinning transforms. Used for LBS xform and point skinning.
    _Task _skinningXformsTask;
    VtMatrix4dArray _skinningXforms;

    /// Inverse tranpose of skinning transforms,
    /// Used for LBS normal skinning.
    _Task _skinningInvTransposeXformsTask;
    VtMatrix3dArray _skinningInvTransposeXforms;

    /// Blend shape weight animation.
    _Task _blendShapeWeightsTask;
    VtFloatArray _blendShapeWeights;

    /// Skel local to world xform. Used for LBS xform and point skinning.
    _Task _skelLocalToWorldXformTask;
    GfMatrix4d _skelLocalToWorldXform;

    /// Mask indicating which indexed times this skel should be processed at.
    std::vector<bool> _timeSampleMask;
};


using _SkelAdapterRefPtr = std::shared_ptr<_SkelAdapter>;


namespace {


bool
_WorldTransformMightBeTimeVarying(const UsdPrim& prim,
                                  UsdGeomXformCache* xformCache)
{
    for (UsdPrim p = prim; !p.IsPseudoRoot(); p = p.GetParent()) {
        if (xformCache->TransformMightBeTimeVarying(p)) {
            return true;
        }
        if (xformCache->GetResetXformStack(p)) {
            break;
        }
    }
    return false;
}


void
_ExtendWorldTransformTimeSamples(const UsdPrim& prim,
                                 const GfInterval& interval,
                                 std::vector<double>* times)
{
    std::vector<double> tmpTimes;
    for (UsdPrim p = prim; !p.IsPseudoRoot(); p = p.GetParent()) {
        if (p.IsA<UsdGeomXformable>()) {
            const UsdGeomXformable xformable(prim);
            const UsdGeomXformable::XformQuery query(xformable);
            if (query.GetTimeSamplesInInterval(interval, &tmpTimes)) {
                times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
            }
            if (query.GetResetXformStack()) {
                break;
            }
        }
    }
}


} // namespace


_SkelAdapter::_SkelAdapter(const UsdSkelBakeSkinningParms& parms,
                           const UsdSkelSkeletonQuery& skelQuery,
                           UsdGeomXformCache* xformCache)
    : _skelQuery(skelQuery)
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(_skelQuery)) {
        return;
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Creating adapter for skel at <%s>\n",
        skelQuery.GetPrim().GetPath().GetText());

    // Activate skinning transform computations if we have a mappable anim,
    // or if restTransforms are authored as a fallback.
    if (parms.deformationFlags & UsdSkelBakeSkinningParms::DeformWithLBS) {
        if (const UsdSkelSkeleton& skel = skelQuery.GetSkeleton()) {
            const auto& animQuery = skelQuery.GetAnimQuery();
            if ((animQuery && !skelQuery.GetMapper().IsNull()) ||
                skel.GetRestTransformsAttr().HasAuthoredValue()) {

                // XXX: Activate computations, but tag them as not required;
                // skinning adapters will tag them as required if needed.
                _skinningXformsTask.SetActive(true, /*required*/ false);
                _skinningInvTransposeXformsTask.SetActive(
                    true, /*required*/ false);

                // The animQuery object may not be valid if the skeleton has a
                // rest transform attribute.
                if (animQuery && animQuery.JointTransformsMightBeTimeVarying()) {
                    _skinningXformsTask.SetMightBeTimeVarying(true);
                    _skinningInvTransposeXformsTask.SetMightBeTimeVarying(true);
                }
                else {
                    _skinningXformsTask.SetMightBeTimeVarying(false);
                    _skinningInvTransposeXformsTask.SetMightBeTimeVarying(false);
                }

                // Also active computation for skel's local to world transform.
                _skelLocalToWorldXformTask.SetActive(true, /*required*/ false);
                _skelLocalToWorldXformTask.SetMightBeTimeVarying(
                    _WorldTransformMightBeTimeVarying(
                        skel.GetPrim(), xformCache));
            }
        }
    }

    // Activate blend shape weight computations if we have authored
    // blend shape anim.
    if (parms.deformationFlags &
        UsdSkelBakeSkinningParms::DeformWithBlendShapes) {

        if (const UsdSkelAnimQuery& animQuery = skelQuery.GetAnimQuery()) {
            // Determine if blend shapes are authored at all.
            std::vector<UsdAttribute> weightAttrs;
            if (animQuery.GetBlendShapeWeightAttributes(&weightAttrs)) {
                _blendShapeWeightsTask.SetActive(
                    std::any_of(weightAttrs.begin(), weightAttrs.end(),
                                [](const UsdAttribute& attr)
                                { return attr.HasAuthoredValue(); }),
                    /*required*/ false);
                _blendShapeWeightsTask.SetMightBeTimeVarying(
                    animQuery.BlendShapeWeightsMightBeTimeVarying());
            }
        }
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]\n  Initial state for skel <%s>:\n"
        "    _skinningXformsTask: %s\n"
        "    _skinningInvTransposeXformsTask: %s\n"
        "    _blendShapeWeightsTask: %s\n"
        "    _skelLocalToWorldXformTask: %s\n",
        GetPrim().GetPath().GetText(),
        _skinningXformsTask.GetDescription().c_str(),
        _skinningInvTransposeXformsTask.GetDescription().c_str(),
        _blendShapeWeightsTask.GetDescription().c_str(),
        _skelLocalToWorldXformTask.GetDescription().c_str());
}


void
_SkelAdapter::ExtendTimeSamples(const GfInterval& interval,
                                std::vector<double>* times)
{
    std::vector<double> tmpTimes;
    if (_skinningXformsTask) {
        if (const auto& animQuery = _skelQuery.GetAnimQuery()) {
            if (animQuery.GetJointTransformTimeSamplesInInterval(
                    interval, &tmpTimes)) {
                times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
            }
        }
    }
    if (_blendShapeWeightsTask) {
        if (const auto& animQuery = _skelQuery.GetAnimQuery()) {
            if (animQuery.GetBlendShapeWeightTimeSamplesInInterval(
                    interval, &tmpTimes)) {
                times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
            }
        }
    }
    if (_skelLocalToWorldXformTask) {
        _ExtendWorldTransformTimeSamples(GetPrim(), interval, times);
    }
}


void
_SkelAdapter::UpdateTransform(const size_t timeIndex,
                              UsdGeomXformCache* xfCache)
{
    TRACE_FUNCTION();

    if (ShouldProcessAtTime(timeIndex)) {

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Updating transform for "
            "<%s> @ time %s\n", GetPrim().GetPath().GetText(),
            TfStringify(xfCache->GetTime()).c_str());

        _skelLocalToWorldXformTask.Run(
            xfCache->GetTime(), GetPrim(), "compute skel local to world xform",
            [&](UsdTimeCode time) {
                _skelLocalToWorldXform =
                    xfCache->GetLocalToWorldTransform(GetPrim());
                return true;
            });
    }
}


void
_SkelAdapter::_ComputeSkinningXforms(const UsdTimeCode time)
{
    _skinningXformsTask.Run(
        time, GetPrim(), "compute skinning xforms",
        [&](UsdTimeCode time) {
            return _skelQuery.ComputeSkinningTransforms(&_skinningXforms, time);
        });
}


void
_SkelAdapter::_ComputeSkinningInvTransposeXforms(const UsdTimeCode time)
{
    if (_skinningXformsTask.HasSampleAtCurrentTime()) {
        _skinningInvTransposeXformsTask.Run(
            time, GetPrim(), "compute skinning inverse transpose xforms",
            [&](UsdTimeCode time) {
                _skinningInvTransposeXforms.resize(_skinningXforms.size());
                const auto skinningXforms = TfMakeConstSpan(_skinningXforms);
                const auto dst = TfMakeSpan(_skinningInvTransposeXforms);
                for (size_t i = 0;i < dst.size(); ++i) {
                    dst[i] = skinningXforms[i].ExtractRotationMatrix()
                        .GetInverse().GetTranspose();
                }
                return true;
            });
    }
}


void
_SkelAdapter::_ComputeBlendShapeWeights(const UsdTimeCode time)
{
    _blendShapeWeightsTask.Run(
        time, GetPrim(), "compute blend shape weights",
        [&](UsdTimeCode time) {
            return _skelQuery.GetAnimQuery().ComputeBlendShapeWeights(
                &_blendShapeWeights, time);
        });
}


void
_SkelAdapter::UpdateAnimation(const UsdTimeCode time, const size_t timeIndex)
{
    TRACE_FUNCTION();

    if (ShouldProcessAtTime(timeIndex)) {

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Updating animation for "
            "<%s> @ time %s\n", GetPrim().GetPath().GetText(),
            TfStringify(time).c_str());

        _ComputeSkinningXforms(time);
        _ComputeSkinningInvTransposeXforms(time);
        _ComputeBlendShapeWeights(time);
    }
}


// ------------------------------------------------------------
// _SkinningAdapter
// ------------------------------------------------------------


/// Object used to store the output of skinning.
/// This object is bound to a single skinnable primitive, and manages
/// both intermediate computations, as well as authoring of final values.
///
/// The overall skinning procedure for a single prim may be summarized as:
/// \code
///     for (time : times) {
///         adapter.Update(time);
///         adapter.Write();
///     }
/// \endcode
///
/// The procedure is split into two calls for the sake of threading:
/// The Update() step may be safely called for different adapters in
/// parallel, whereas writes for each layer must be called in serial.
struct _SkinningAdapter
{
    /// Flags indicating which deformation paths are active.
    enum ComputationFlags {
        RequiresSkinningXforms =
            UsdSkelBakeSkinningParms::DeformWithLBS,
        RequiresSkinningInvTransposeXforms =
            UsdSkelBakeSkinningParms::DeformNormalsWithLBS,
        RequiresBlendShapeWeights = 
            UsdSkelBakeSkinningParms::DeformWithBlendShapes,
        RequiresGeomBindXform =
            UsdSkelBakeSkinningParms::DeformWithLBS,
        RequiresGeomBindInvTransposeXform =
            UsdSkelBakeSkinningParms::DeformNormalsWithLBS,
        RequiresJointInfluences =
            UsdSkelBakeSkinningParms::DeformWithLBS,
        RequiresSkelLocalToWorldXform =
            UsdSkelBakeSkinningParms::DeformWithLBS,
        RequiresPrimLocalToWorldXform =
            (UsdSkelBakeSkinningParms::DeformPointsWithLBS|
             UsdSkelBakeSkinningParms::DeformNormalsWithLBS),
        RequiresPrimParentToWorldXform =
            UsdSkelBakeSkinningParms::DeformXformWithLBS
    };

    _SkinningAdapter(const UsdSkelBakeSkinningParms& parms,
                     const UsdSkelSkinningQuery& skinningQuery,
                     const _SkelAdapterRefPtr& skelAdapter,
                     const SdfLayerHandle& layer,
                     const unsigned layerIndex,
                     UsdGeomXformCache* xformCache);

    /// Returns the skel adapter that manages skel animation associated with
    /// this adapter.
    const _SkelAdapterRefPtr& GetSkelAdapter() const {
        return _skelAdapter;
    }

    const UsdPrim& GetPrim() const {
        return _skinningQuery.GetPrim();
    }

    unsigned GetLayerIndex() const {
        return _layerIndex;
    }

    bool ShouldProcessAtTime(const size_t timeIndex) const {
        return _flags && _skelAdapter->ShouldProcessAtTime(timeIndex);
    }

    /// Append additional time samples of the skel to \p times.
    void ExtendTimeSamples(const GfInterval& interval,
                           std::vector<double>* times);

    /// Use \p xfCache to update cached transform data at the \p timeIndex'th
    /// time sample. Cached values are stored only if necessary.
    void UpdateTransform(const size_t timeIndex, UsdGeomXformCache* xfCache);

    void Update(const UsdTimeCode time, const size_t timeIndex);

    /// Write computed values to the SdfLayer.
    /// Returns the byte count of the data being written, ignoring
    /// any data sharing considerations.
    size_t Write(const UsdTimeCode time, const size_t timeIndex);

    bool HasTasksToRun() const { return _flags; }

    /// Returns true if the extent of the skinned prim must be updated
    /// separately, after skinning is completed.
    bool RequiresPostExtentUpdate() const {
        return _flags & UsdSkelBakeSkinningParms::ModifiesPoints
            && !_extentWriter;
    }

private:

    bool _ComputeRestPoints(const UsdTimeCode time);
    bool _ComputeRestNormals(const UsdTimeCode time);
    bool _ComputeGeomBindXform(const UsdTimeCode time);
    bool _ComputeJointInfluences(const UsdTimeCode time);

    void _DeformWithBlendShapes();

    void _DeformWithLBS(const UsdTimeCode time, const size_t timeIndex);

    void _DeformPointsWithLBS(const GfMatrix4d& skelToGprimXform);

    void _DeformNormalsWithLBS(const GfMatrix4d& skelToGprimXform);

    void _DeformXformWithLBS(const GfMatrix4d& skelLocalToWorldXform);

private:
    
    UsdSkelSkinningQuery _skinningQuery;
    _SkelAdapterRefPtr _skelAdapter;

    int _flags = 0;

    unsigned _layerIndex;

    // Blend shape bindings.
    std::shared_ptr<UsdSkelBlendShapeQuery> _blendShapeQuery;
    std::vector<VtIntArray> _blendShapePointIndices;
    std::vector<VtVec3fArray> _subShapePointOffsets;
    std::vector<VtVec3fArray> _subShapeNormalOffsets;

    // Rest points.
    _Task _restPointsTask;
    VtVec3fArray _restPoints;
    UsdAttributeQuery _restPointsQuery;

    // Rest normals.
    _Task _restNormalsTask;
    VtVec3fArray _restNormals;
    UsdAttributeQuery _restNormalsQuery;

    // Geom bind transform.
    _Task _geomBindXformTask;
    GfMatrix4d _geomBindXform;
    UsdAttributeQuery _geomBindXformQuery;

    // Inverse transpose of the geom bind xform.
    _Task _geomBindInvTransposeXformTask;
    GfMatrix3d _geomBindInvTransposeXform;

    // Joint influences.
    _Task _jointInfluencesTask;
    VtIntArray _jointIndices;
    VtFloatArray _jointWeights;

    // Local to world gprim xform.
    // Used for LBS point/normal skinning only.
    _Task _localToWorldXformTask;
    GfMatrix4d _localToWorldXform;

    // Parent to world gprim xform.
    // Used for LBS xform skinning.
    _Task _parentToWorldXformTask;
    GfMatrix4d _parentToWorldXform;

    // Computed outputs and associated writers.

    // Deformed points.
    _OutputHolder<VtVec3fArray> _points;
    _AttrWriter _pointsWriter;

    // Deformed normals.
    _OutputHolder<VtVec3fArray> _normals;
    _AttrWriter _normalsWriter;

    // Point extent (UsdGeomMesh prims only).
    _OutputHolder<VtVec3fArray> _extent;
    _AttrWriter _extentWriter;

    // Deformed xform.
    _OutputHolder<GfMatrix4d> _xform;
    _AttrWriter _xformWriter;
};


using _SkinningAdapterRefPtr = std::shared_ptr<_SkinningAdapter>;


_SkinningAdapter::_SkinningAdapter(
    const UsdSkelBakeSkinningParms& parms,
    const UsdSkelSkinningQuery& skinningQuery,
    const _SkelAdapterRefPtr& skelAdapter,
    const SdfLayerHandle& layer,
    const unsigned layerIndex,
    UsdGeomXformCache* xformCache)
    : _skinningQuery(skinningQuery),
      _skelAdapter(skelAdapter), _layerIndex(layerIndex)
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(skinningQuery) || !TF_VERIFY(skelAdapter)) {
        return;
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Creating adapter for skinning prim at <%s>\n",
        skinningQuery.GetPrim().GetPath().GetText());

    const bool isPointBased = skinningQuery.GetPrim().IsA<UsdGeomPointBased>();
    const bool isXformable =
        isPointBased || skinningQuery.GetPrim().IsA<UsdGeomXformable>();

    // Get normal/point queries, but only if authored.
    if (isPointBased) {

        const UsdGeomPointBased pointBased(skinningQuery.GetPrim());

        if (parms.deformationFlags & UsdSkelBakeSkinningParms::ModifiesPoints) {
            _restPointsQuery = UsdAttributeQuery(pointBased.GetPointsAttr());
            if (!_restPointsQuery.HasAuthoredValue()) {
                _restPointsQuery = UsdAttributeQuery();
            }
        }
        if (parms.deformationFlags &
            UsdSkelBakeSkinningParms::ModifiesNormals) {
            _restNormalsQuery = UsdAttributeQuery(pointBased.GetNormalsAttr());
            const TfToken& normalsInterp = pointBased.GetNormalsInterpolation();
            // Can only process vertex/varying normals.
            if (!_restNormalsQuery.HasAuthoredValue() ||
                (normalsInterp != UsdGeomTokens->vertex &&
                 normalsInterp != UsdGeomTokens->varying)) {
                _restNormalsQuery = UsdAttributeQuery();
            }
        }
    }

    // LBS Skinning.
    if ((parms.deformationFlags & UsdSkelBakeSkinningParms::DeformWithLBS) &&
        skinningQuery.HasJointInfluences()) {

        if (skinningQuery.IsRigidlyDeformed() && isXformable) {
            if ((parms.deformationFlags &
                 UsdSkelBakeSkinningParms::DeformXformWithLBS) &&
                skelAdapter->CanComputeSkinningXforms()) {
                _flags |= UsdSkelBakeSkinningParms::DeformXformWithLBS;
            }
        } else if (isPointBased) {
            if ((parms.deformationFlags &
                 UsdSkelBakeSkinningParms::DeformPointsWithLBS) &&
                _restPointsQuery.IsValid() &&
                skelAdapter->CanComputeSkinningXforms()) {
                _flags |= UsdSkelBakeSkinningParms::DeformPointsWithLBS;
            }
            if ((parms.deformationFlags &
                 UsdSkelBakeSkinningParms::DeformNormalsWithLBS) &&
                _restNormalsQuery.IsValid() &&
                skelAdapter->CanComputeSkinningInvTransposeXforms()) {
                _flags |= UsdSkelBakeSkinningParms::DeformNormalsWithLBS;
            }
        }
    }

    // Blend shapes.
    if ((parms.deformationFlags &
         UsdSkelBakeSkinningParms::DeformWithBlendShapes) &&
        skelAdapter->CanComputeBlendShapeWeights() &&
        isPointBased && skinningQuery.HasBlendShapes() &&
        (_restPointsQuery || _restNormalsQuery)) {
        
        // Create a blend shape query to help process blend shapes.
        _blendShapeQuery.reset(new UsdSkelBlendShapeQuery(
                                   UsdSkelBindingAPI(skinningQuery.GetPrim())));
        if (_blendShapeQuery->IsValid()) {
            if ((parms.deformationFlags & 
                 UsdSkelBakeSkinningParms::DeformPointsWithBlendShapes) &&
                _restPointsQuery) {

                _subShapePointOffsets =
                    _blendShapeQuery->ComputeSubShapePointOffsets();

                const bool hasPointOffsets =
                    std::any_of(_subShapePointOffsets.begin(),
                                _subShapePointOffsets.end(),
                                [](const VtVec3fArray& points)
                                { return !points.empty(); });
                if (hasPointOffsets) {
                    _flags |=
                        UsdSkelBakeSkinningParms::DeformPointsWithBlendShapes;
                }
            }
            if ((parms.deformationFlags &
                 UsdSkelBakeSkinningParms::DeformNormalsWithBlendShapes) &&
                _restNormalsQuery) {

                _subShapeNormalOffsets =
                    _blendShapeQuery->ComputeSubShapeNormalOffsets();
                const bool hasNormalOffsets =
                    std::any_of(_subShapeNormalOffsets.begin(),
                                _subShapeNormalOffsets.end(),
                                [](const VtVec3fArray& normals)
                                { return !normals.empty(); });
                if (hasNormalOffsets) {
                    _flags |=
                        UsdSkelBakeSkinningParms::DeformNormalsWithBlendShapes;
                }
            }
            if (_flags & UsdSkelBakeSkinningParms::DeformWithBlendShapes) {
                _blendShapePointIndices =
                    _blendShapeQuery->ComputeBlendShapePointIndices();
            }
        }
        if (!(_flags & UsdSkelBakeSkinningParms::DeformWithBlendShapes)) {
            _blendShapeQuery.reset();
        }
    }

    if (!_flags) {

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   No computations active "
            "for <%s>: skipping\n",
            skinningQuery.GetPrim().GetPath().GetText());

        return;
    }

    // Create output specs. Disable any procedures that can't run
    // if spec creation fails.
    const SdfPrimSpecHandle primSpec =
        SdfCreatePrimInLayer(layer, skinningQuery.GetPrim().GetPath());
    if (!TF_VERIFY(primSpec)) {
        _flags = 0;
        return;
    }
    if (_flags & UsdSkelBakeSkinningParms::ModifiesPoints) {
        if (_pointsWriter.Define(primSpec, UsdGeomTokens->points,
                                 SdfValueTypeNames->Point3fArray,
                                 SdfVariabilityVarying)) {
            if (parms.updateExtents &&
                _skinningQuery.GetPrim().IsA<UsdGeomMesh>()) {
                // As an optimization, we directly modify extent
                // of UsdGeomMesh prims. All other skinned prims
                // are updated at the end of skinning, using
                // extents plugings.
                _extentWriter.Define(primSpec, UsdGeomTokens->extent,
                                     SdfValueTypeNames->Float3Array,
                                     SdfVariabilityVarying);
            }
        } else {
            _flags &= ~UsdSkelBakeSkinningParms::ModifiesPoints;
        }
    }
    if (_flags & UsdSkelBakeSkinningParms::ModifiesNormals) {
        if (!_normalsWriter.Define(primSpec, UsdGeomTokens->normals,
                                   SdfValueTypeNames->Normal3fArray,
                                   SdfVariabilityVarying)) {
            _flags &= ~UsdSkelBakeSkinningParms::ModifiesNormals;
        }
    }
    if (_flags & UsdSkelBakeSkinningParms::ModifiesXform) {
        _AttrWriter xformOpOrderWriter;
        if (_xformWriter.Define(primSpec, _tokens->xformOpTransform,
                                SdfValueTypeNames->Matrix4d,
                                SdfVariabilityVarying) &&
            xformOpOrderWriter.Define(primSpec, UsdGeomTokens->xformOpOrder,
                                      SdfValueTypeNames->TokenArray,
                                      SdfVariabilityUniform)) {
            static const VtTokenArray matrixXformOpOrder(
                {_tokens->xformOpTransform});
            xformOpOrderWriter.Set(matrixXformOpOrder, UsdTimeCode::Default());
        } else {
            _flags &= ~UsdSkelBakeSkinningParms::ModifiesXform;
        }
    }

    // Activate computations.

    if (_flags & UsdSkelBakeSkinningParms::ModifiesPoints) {
        // Will need rest points.
        _restPointsTask.SetActive(true);
        _restPointsTask.SetMightBeTimeVarying(
            _restPointsQuery.ValueMightBeTimeVarying());
    }
    
    if (_flags & UsdSkelBakeSkinningParms::ModifiesNormals) {
        // Will need rest normals.
        _restNormalsTask.SetActive(true);
        _restNormalsTask.SetMightBeTimeVarying(
            _restNormalsQuery.ValueMightBeTimeVarying());
    }

    if (_flags & RequiresGeomBindXform) {
        _geomBindXformTask.SetActive(true);
        if ((_geomBindXformQuery = UsdAttributeQuery(
                 _skinningQuery.GetGeomBindTransformAttr()))) {
            _geomBindXformTask.SetMightBeTimeVarying(
                _geomBindXformQuery.ValueMightBeTimeVarying());
        }

        if (_flags & RequiresGeomBindInvTransposeXform) {
            _geomBindInvTransposeXformTask.SetActive(true);
            _geomBindInvTransposeXformTask.SetMightBeTimeVarying(
                _geomBindXformTask.MightBeTimeVarying());
        }
    }
    
    if (_flags & RequiresJointInfluences) {
        _jointInfluencesTask.SetActive(true);
        _jointInfluencesTask.SetMightBeTimeVarying(
            _skinningQuery.GetJointIndicesPrimvar().ValueMightBeTimeVarying() ||
            _skinningQuery.GetJointWeightsPrimvar().ValueMightBeTimeVarying());
    }
    
    if (_flags & RequiresPrimLocalToWorldXform) {
        _localToWorldXformTask.SetActive(true);
        _localToWorldXformTask.SetMightBeTimeVarying(
            _WorldTransformMightBeTimeVarying(
                skinningQuery.GetPrim(), xformCache));
    }

    if (_flags & RequiresPrimParentToWorldXform) {

        if (!xformCache->GetResetXformStack(skinningQuery.GetPrim())) {
            _parentToWorldXformTask.SetActive(true);
            _parentToWorldXformTask.SetMightBeTimeVarying(
                _WorldTransformMightBeTimeVarying(
                    skinningQuery.GetPrim().GetParent(), xformCache));
        } else {
            // Parent xform will always be identity.
            // Initialize the parent xform, but keep the computation inactive.
            _parentToWorldXform.SetIdentity();
        }
    }

    // Mark dependent computations on the skel as required where needed.
    if (_flags & RequiresBlendShapeWeights) {
        skelAdapter->SetBlendShapeWeightsRequired(true);
    }
    if (_flags & RequiresSkinningXforms) {
        skelAdapter->SetSkinningXformsRequired(true);
    }
    if (_flags & RequiresSkinningInvTransposeXforms) {
        skelAdapter->SetSkinningInvTransposeXformsRequired(true);
    }
    if (_flags & RequiresSkelLocalToWorldXform) {
        skelAdapter->SetLocalToWorldXformRequired(true);
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]\n  Computation state for skinnable prim <%s>:\n"
        "    _restPointsTask: %s\n"
        "    _restNormalsTask: %s\n"
        "    _geomBindXformTask: %s\n"
        "    _geomBindInvTransposeXformTask: %s\n"
        "    _jointInfluencesTask: %s\n"
        "    _localToWorldXformTask: %s\n"
        "    _parentToWorldXformTask: %s\n"
        "  Deformation flags:\n%s",
        _skinningQuery.GetPrim().GetPath().GetText(),
        _restPointsTask.GetDescription().c_str(),
        _restNormalsTask.GetDescription().c_str(),
        _geomBindXformTask.GetDescription().c_str(),
        _geomBindInvTransposeXformTask.GetDescription().c_str(),
        _jointInfluencesTask.GetDescription().c_str(),
        _localToWorldXformTask.GetDescription().c_str(),
        _parentToWorldXformTask.GetDescription().c_str(),
        _DeformationFlagsToString(_flags, "    ").c_str());
}


void
_SkinningAdapter::ExtendTimeSamples(const GfInterval& interval,
                                    std::vector<double>* times)
{
    std::vector<double> tmpTimes;
    if (_restPointsTask) {
        if (_restPointsQuery.GetTimeSamplesInInterval(
                interval, &tmpTimes)) {
            times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
        }
    }
    if (_restNormalsTask) {
        if (_restNormalsQuery.GetTimeSamplesInInterval(
                interval, &tmpTimes)) {
            times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
        }
    }
    if (_geomBindXformTask && _geomBindXformQuery) {
        if (_geomBindXformQuery.GetTimeSamplesInInterval(
                interval, &tmpTimes)) {
            times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
        }
    }
    if (_jointInfluencesTask) {
        for (const auto& pv : {_skinningQuery.GetJointIndicesPrimvar(),
                               _skinningQuery.GetJointWeightsPrimvar()}) {
            if (pv.GetTimeSamplesInInterval(
                    interval, &tmpTimes)) {
                times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
            }
        }
    }
    if (_localToWorldXformTask) {
        _ExtendWorldTransformTimeSamples(_skinningQuery.GetPrim(),
                                         interval, times);
    }
    if  (_parentToWorldXformTask) {
        _ExtendWorldTransformTimeSamples(_skinningQuery.GetPrim().GetParent(),
                                         interval, times);
    }
}


size_t
_SkinningAdapter::Write(const UsdTimeCode time, const size_t timeIndex)
{
    TRACE_FUNCTION();

    if (!ShouldProcessAtTime(timeIndex)) {
        return 0;
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Write data for <%s> @ time %s\n",
        GetPrim().GetPath().GetText(), TfStringify(time).c_str());

    size_t bytesStored = 0;
    if (_points.hasSampleAtCurrentTime) {
        bytesStored += _pointsWriter.Set(_points.value, time);
    }
    if (_normals.hasSampleAtCurrentTime) {
        bytesStored += _normalsWriter.Set(_normals.value, time);
    }
    if (_extent.hasSampleAtCurrentTime) {
        bytesStored += _extentWriter.Set(_extent.value, time);
    }
    if (_xform.hasSampleAtCurrentTime) {
        bytesStored += _xformWriter.Set(_xform.value, time);
    }
    return bytesStored;
}


void
_SkinningAdapter::UpdateTransform(const size_t timeIndex,
                                  UsdGeomXformCache* xfCache)
{
    TRACE_FUNCTION();

    if (ShouldProcessAtTime(timeIndex)) {

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Updating transforms for "
            "<%s> @ time %s\n",
            GetPrim().GetPath().GetText(),
            TfStringify(xfCache->GetTime()).c_str());

        _localToWorldXformTask.Run(
            xfCache->GetTime(), GetPrim(), "compute prim local to world xform",
            [&](UsdTimeCode time) {
                _localToWorldXform =
                    xfCache->GetLocalToWorldTransform(GetPrim());
                return true;
            });

        _parentToWorldXformTask.Run(
            xfCache->GetTime(), _skinningQuery.GetPrim(),
            "compute prim parent to world xform",
            [&](UsdTimeCode time) {
                _parentToWorldXform =
                    xfCache->GetParentToWorldTransform(
                        _skinningQuery.GetPrim());
                return true;
            });
    }
}


bool
_SkinningAdapter::_ComputeRestPoints(const UsdTimeCode time)
{
    return _restPointsTask.Run(
        time, GetPrim(), "compute rest points",
        [&](UsdTimeCode time) -> bool {
            return _restPointsQuery.Get(&_restPoints, time);
        });
}


bool
_SkinningAdapter::_ComputeRestNormals(const UsdTimeCode time)
{
    return _restNormalsTask.Run(
        time, GetPrim(), "compute rest normals",
        [&](UsdTimeCode time) {
            return _restNormalsQuery.Get(&_restNormals, time);
        });
}


bool
_SkinningAdapter::_ComputeGeomBindXform(const UsdTimeCode time)
{
    _geomBindXformTask.Run(
        time, GetPrim(), "compute geom bind xform",
        [&](UsdTimeCode time) {
            _geomBindXform = _skinningQuery.GetGeomBindTransform(time);
            return true;
        });
    if (_geomBindXformTask.HasSampleAtCurrentTime()) {
        _geomBindInvTransposeXformTask.Run(
            time, GetPrim(),
            "compute geom bind inverse transpose xform",
            [&](UsdTimeCode time) {
                _geomBindInvTransposeXform =
                    _geomBindXform.ExtractRotationMatrix()
                    .GetInverse().GetTranspose();
                return true;
            });
    }
    return true;
}


bool
_SkinningAdapter::_ComputeJointInfluences(const UsdTimeCode time)
{
    return _jointInfluencesTask.Run(
        time, GetPrim(), "compute joint influences",
        [&](UsdTimeCode time) {
            return _skinningQuery.ComputeJointInfluences(
                &_jointIndices, &_jointWeights, time);
        });
}


void
_SkinningAdapter::_DeformWithBlendShapes()
{
    VtFloatArray weights;
    if (_blendShapeQuery && _skelAdapter->GetBlendShapeWeights(&weights)) {
        // Remap the wegiht anim into the order for this prim.
        VtFloatArray weightsForPrim;    
        if (_skinningQuery.GetBlendShapeMapper()->Remap(
                weights, &weightsForPrim)) {

            // Resolve sub shapes (I.e., in-betweens)
            VtFloatArray subShapeWeights;
            VtUIntArray blendShapeIndices, subShapeIndices;
            if (_blendShapeQuery->ComputeSubShapeWeights(
                    weightsForPrim, &subShapeWeights,
                    &blendShapeIndices, &subShapeIndices)) {

                if (_flags & UsdSkelBakeSkinningParms::
                    DeformPointsWithBlendShapes) {

                    // Initialize points to rest if not yet initialized.
                    if (!_points.hasSampleAtCurrentTime) {
                        _points.value = _restPoints;
                    }

                    _points.hasSampleAtCurrentTime =
                        _blendShapeQuery->ComputeDeformedPoints(
                            subShapeWeights, blendShapeIndices,
                            subShapeIndices, _blendShapePointIndices,
                            _subShapePointOffsets, _points.value);
                }
                if (_flags & UsdSkelBakeSkinningParms::
                    DeformNormalsWithBlendShapes) {

                    // Initialize normals to rest if not yet initialized.
                    if (!_normals.hasSampleAtCurrentTime) {
                        _normals.value = _restNormals;
                    }
                    _normals.hasSampleAtCurrentTime =
                        _blendShapeQuery->ComputeDeformedNormals(
                            subShapeWeights, blendShapeIndices,
                            subShapeIndices, _blendShapePointIndices,
                            _subShapeNormalOffsets, _normals.value);
                }
            }
        }
    }
}


void
_SkinningAdapter::_DeformWithLBS(const UsdTimeCode time, const size_t timeIndex)
{
    if (!_ComputeGeomBindXform(time) || !_ComputeJointInfluences(time)) {
        return;
    }

    GfMatrix4d skelLocalToWorldXform;
    if (!_skelAdapter->GetLocalToWorldTransform(&skelLocalToWorldXform)) {
        return;
    }
    
    if (_flags & (UsdSkelBakeSkinningParms::DeformPointsWithLBS |
                  UsdSkelBakeSkinningParms::DeformNormalsWithLBS)) {
        
        // Skinning deforms points/normals in *skel* space.
        // A world-space point is then computed as:
        //
        //    worldSkinnedPoint = skelSkinnedPoint * skelLocalToWorld
        //
        // Since we're baking points/noramls into a gprim, we must
        // transform these from skel space into gprim space, such that:
        //
        //    localSkinnedPoint * gprimLocalToWorld = worldSkinnedPoint
        //  
        // So the points/normals we store must be transformed as:
        //
        //    localSkinnedPoint = skelSkinnedPoint *
        //       skelLocalToWorld * inv(gprimLocalToWorld)

        TF_VERIFY(_localToWorldXformTask.HasSampleAtCurrentTime());

        const GfMatrix4d skelToGprimXform =
            skelLocalToWorldXform * _localToWorldXform.GetInverse();

        if (_flags & UsdSkelBakeSkinningParms::DeformPointsWithLBS) {
            _DeformPointsWithLBS(skelToGprimXform);
        }
        if (_flags & UsdSkelBakeSkinningParms::DeformNormalsWithLBS) {
            _DeformNormalsWithLBS(skelToGprimXform);
        }
    } else if (_flags & UsdSkelBakeSkinningParms::DeformXformWithLBS) {
        _DeformXformWithLBS(skelLocalToWorldXform);
    }
}


void
_SkinningAdapter::_DeformPointsWithLBS(const GfMatrix4d& skelToGprimXf)
{
    TRACE_FUNCTION();

    if (!_restPointsTask.HasSampleAtCurrentTime() ||
        !_jointInfluencesTask.HasSampleAtCurrentTime()) {
        return;
    }

    VtMatrix4dArray xforms;
    if (!_skelAdapter->GetSkinningTransforms(&xforms)) {
        return;
    }

    // Handle local skel:joints ordering.
    VtMatrix4dArray xformsForPrim;
    if (_skinningQuery.GetJointMapper()) {
        if (!_skinningQuery.GetJointMapper()->RemapTransforms(
                xforms, &xformsForPrim)) {
            return;
        }
     } else {
        // No mapper; use the same joint order as given on the skel.
        xformsForPrim = xforms;
    }

    // Initialize points from rest points.
    // Keep the current points if already initialized
    // (eg., by blendshape application)
    if (!_points.hasSampleAtCurrentTime) {
        _points.value = _restPoints;
    }

    _points.hasSampleAtCurrentTime =
        UsdSkelSkinPointsLBS(_geomBindXform, xformsForPrim,
                             _jointIndices, _jointWeights,
                             _skinningQuery.GetNumInfluencesPerComponent(),
                             _points.value);
    if (!_points.hasSampleAtCurrentTime) {
        return;
    }

    // Output of skinning is in *skel* space.
    // Transform the result into gprim space.

    WorkParallelForEach(
        _points.value.begin(), _points.value.end(),
        [&skelToGprimXf](GfVec3f& pt) {
            pt = skelToGprimXf.Transform(pt);
        });
}


void
_SkinningAdapter::_DeformNormalsWithLBS(const GfMatrix4d& skelToGprimXf)
{
    TRACE_FUNCTION();

    if (!_restNormalsTask.HasSampleAtCurrentTime() ||
        !_jointInfluencesTask.HasSampleAtCurrentTime()) {
        return;
    }

    VtMatrix3dArray xforms;
    if (!_skelAdapter->GetSkinningInvTransposeTransforms(&xforms)) {
        return;
    }

    // Handle local skel:joints ordering.
    VtMatrix3dArray xformsForPrim;
    if (_skinningQuery.GetJointMapper()) {
        static const GfMatrix3d identity(1);
        if (!_skinningQuery.GetJointMapper()->Remap(
                xforms, &xformsForPrim, /*elemSize*/ 1, &identity)) {
            return;
        }
     } else {
        // No mapper; use the same joint order as given on the skel.
        xformsForPrim = xforms;
    }

    // Initialize normals from rest normals.
    // Keep the current normals if already initialized
    // (eg., by blendshape application)
    if (!_normals.hasSampleAtCurrentTime) {
        _normals.value = _restNormals;
    }

    _normals.hasSampleAtCurrentTime =
        UsdSkelSkinNormalsLBS(_geomBindInvTransposeXform, xformsForPrim,
                              _jointIndices, _jointWeights,
                              _skinningQuery.GetNumInfluencesPerComponent(),
                              _normals.value);
    if (!_normals.hasSampleAtCurrentTime) {
        return;
    }

    // Output of skinning is in *skel* space.
    // Transform the result into gprim space.

    const GfMatrix3d& skelToGprimInvTransposeXform =
        skelToGprimXf.ExtractRotationMatrix().GetInverse().GetTranspose();

    WorkParallelForEach(
        _normals.value.begin(), _normals.value.end(),
        [&skelToGprimInvTransposeXform](GfVec3f& n) {
            n = n * skelToGprimInvTransposeXform;
        });
}


void
_SkinningAdapter::_DeformXformWithLBS(const GfMatrix4d& skelLocalToWorldXform)
{
    TRACE_FUNCTION();

    if (!_jointInfluencesTask.HasSampleAtCurrentTime() ||
        !_geomBindXformTask.HasSampleAtCurrentTime()) {
        return;
    }

    VtMatrix4dArray xforms;
    if (!_skelAdapter->GetSkinningTransforms(&xforms)) {
        return;
    }

    // Handle local skel:joints ordering.
    VtMatrix4dArray xformsForPrim;
    if (_skinningQuery.GetJointMapper()) {
        if (!_skinningQuery.GetJointMapper()->RemapTransforms(
                xforms, &xformsForPrim)) {
            return;
        }
    } else {
        // No mapper; use the same joint order as given on the skel.
        xformsForPrim = xforms;
    }

    _xform.hasSampleAtCurrentTime =
        UsdSkelSkinTransformLBS(_geomBindXform, xformsForPrim,
                                _jointIndices, _jointWeights,
                                &_xform.value);
    
    if (!_xform.hasSampleAtCurrentTime) {
        return;
    }

    // Skinning a transform produces a new transform in *skel* space.
    // A world-space transform is then computed as:
    //
    //    worldSkinnedXform = skelSkinnedXform * skelLocalToWorld
    //
    // Since we're baking transforms into a prim, we must transform
    // from skel space into the space of that prim's parent, such that:
    //
    //    newLocalXform * parentToWorld = worldSkinnedXform
    //
    // So the skinned, local transform becomes:
    //
    //    newLocalXform = skelSkinnedXform *
    //        skelLocalToWorld * inv(parentToWorld)

    _xform.value = _xform.value * skelLocalToWorldXform *
        _parentToWorldXform.GetInverse();
}


void
_SkinningAdapter::Update(const UsdTimeCode time, const size_t timeIndex)
{
    TRACE_FUNCTION();

    if (!ShouldProcessAtTime(timeIndex)) {
        return;
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Updating skinning for "
        "<%s> @ time %s\n",
        GetPrim().GetPath().GetText(),
        TfStringify(time).c_str());

    _points.BeginUpdate();
    _normals.BeginUpdate();
    _extent.BeginUpdate();
    _xform.BeginUpdate();

    // Compute inputs.
    _ComputeRestPoints(time);
    _ComputeRestNormals(time);

    // Blend shapes precede LBS skinning.
    if (_flags & UsdSkelBakeSkinningParms::DeformWithBlendShapes) {
        _DeformWithBlendShapes();
    }

    if (_flags & UsdSkelBakeSkinningParms::DeformWithLBS) {
        _DeformWithLBS(time, timeIndex);
    }

    // If a valid points sample was computed, also compute a new extent.
    if (_points.hasSampleAtCurrentTime && _extentWriter) {
        _extent.hasSampleAtCurrentTime =
            UsdGeomPointBased::ComputeExtent(_points.value, &_extent.value);
    }
}


void
_UnionTimes(const std::vector<double> additionalTimes,
            std::vector<double>* times,
            std::vector<double>* tmpUnionTimes)
{
    tmpUnionTimes->resize(times->size() + additionalTimes.size());
    const auto& it = std::set_union(times->begin(), times->end(),
                                    additionalTimes.begin(),
                                    additionalTimes.end(),
                                    tmpUnionTimes->begin());
    tmpUnionTimes->resize(std::distance(tmpUnionTimes->begin(), it));
    times->swap(*tmpUnionTimes);
}


/// Create skel and skinning adapters from UsdSkelBinding objects to help
/// wrangle I/O.
bool
_CreateAdapters(
    const UsdSkelBakeSkinningParms& parms,
    const UsdSkelCache& skelCache,
    std::vector<_SkelAdapterRefPtr>* skelAdapters,
    std::vector<_SkinningAdapterRefPtr>* skinningAdapters,
    UsdGeomXformCache* xfCache)
{
    if (parms.bindings.size() != parms.layerIndices.size()) {
        TF_CODING_ERROR("size of parms.bindings [%zu] != "
                        "size of parms.bindingLayerIndices [%zu]",
                        parms.bindings.size(), parms.layerIndices.size());
        return false;
    }

    skelAdapters->reserve(parms.bindings.size());
    skinningAdapters->reserve(parms.bindings.size());

    for (size_t i = 0; i < parms.bindings.size(); ++i) {
        const unsigned layerIndex = parms.layerIndices[i];
        if (layerIndex >= parms.layers.size()) {
            TF_WARN("Layer index %d for the %zu'th binding "
                    ">= number of layers [%zu]",
                    layerIndex, i, parms.layers.size());
            continue;
        }

        const SdfLayerHandle& layer = parms.layers[layerIndex];
        if (!layer) {
            continue;
        }

        const UsdSkelBinding& binding = parms.bindings[i];
        if (!binding.GetSkinningTargets().empty()) {

            if (const UsdSkelSkeletonQuery skelQuery =
                skelCache.GetSkelQuery(binding.GetSkeleton())) {
                
                auto skelAdapter =
                    std::make_shared<_SkelAdapter>(parms, skelQuery, xfCache);

                const unsigned layerIndex = parms.layerIndices[i];

                for (const UsdSkelSkinningQuery& skinningQuery :
                         binding.GetSkinningTargets()) {

                    auto skinningAdapter =  
                        std::make_shared<_SkinningAdapter>(
                            parms, skinningQuery, skelAdapter,
                            layer, layerIndex,xfCache);
                    
                    // Only add this adapter if it will be used.
                    if (skinningAdapter->HasTasksToRun()) {
                        skinningAdapters->push_back(skinningAdapter);
                    }
                }

                if (skelAdapter->HasTasksToRun()) {
                    skelAdapters->push_back(skelAdapter);
                }
            }
        }
    }
    return true;
}


/// Compute an array of time samples over \p interval.
/// The samples are added based on the expected sampling rate for playback.
/// I.e., the exact set of time codes that we expect to be queried when
/// the stage is played back at its configured
/// timeCodesPerSecond/framesPerSecond rate.
std::vector<double>
_GetStagePlaybackTimeCodesInRange(const UsdStagePtr& stage,
                                  const GfInterval& interval)
{
    std::vector<double> times;
    if (!stage->HasAuthoredTimeCodeRange()) {
        return times;
    }
    
    const double timeCodesPerSecond = stage->GetTimeCodesPerSecond();
    const double framesPerSecond = stage->GetFramesPerSecond();
    if (GfIsClose(timeCodesPerSecond, 0.0, 1e-6) ||
        GfIsClose(framesPerSecond, 0.0, 1e-6)) {
        return times;
    }
    // Compute the expected per-frame time step for playback.
    const double timeStep =
        std::abs(timeCodesPerSecond/framesPerSecond);

    const double stageStart = stage->GetStartTimeCode();
    const double stageEnd = stage->GetEndTimeCode();
    if (stageEnd < stageStart) {
        // Malfored time code range.
        return times;
    }
    // Add 1 to the sample count for an inclusive range.
    const int64_t numTimeSamples = (stageEnd-stageStart)/timeStep + 1;
    times.reserve(numTimeSamples);
    for(int64_t i = 0; i <= numTimeSamples; ++i) {
        // Add samples based on integer multiples of the time step
        // to reduce error.
        const double t = stageStart + timeStep*i;
        if (interval.Contains(t)) {
            times.push_back(t);
        }
    }
    return times;
}


/// Compute the full set of time samples at which data must be sampled.
/// A mask is applied to each SkelAdapter indicating at what times within
/// that full set of time samples the SkelAdapter should be processed.
std::vector<UsdTimeCode>
_ComputeTimeSamples(
    const UsdStagePtr& stage,
    const GfInterval& interval,
    const std::vector<_SkelAdapterRefPtr>& skelAdapters,
    const std::vector<_SkinningAdapterRefPtr>& skinningAdapters,
    UsdGeomXformCache* xfCache)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Computing time samples for skinning\n");

    std::vector<UsdTimeCode> times;

    // Pre-compute time samples for each skel adapter.
    std::unordered_map<_SkelAdapterRefPtr, std::vector<double> > skelTimesMap;

    // Pre-populate the skelTimesMap on a single thread. Each worker thread
    // will only access its own members in this map, so access to each vector
    // is safe.
    for (const _SkelAdapterRefPtr &adapter : skelAdapters)
        skelTimesMap[adapter] = std::vector<double>();

    WorkParallelForN(
        skelAdapters.size(),
        [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                skelAdapters[i]->ExtendTimeSamples(
                    interval, &skelTimesMap[skelAdapters[i]]);
            }
        });

    // Extend the time samples of each skel adapter with the time samples
    // of each skinning adapter.
    // NOTE: multiple skinning adapters may share the same skel adapter, so in
    // order for this work to be done in parallel the skinning adapters would
    // need to be grouped such that that the same skel adapter isn't modified
    // by multiple threads.
    for (const _SkinningAdapterRefPtr &adapter : skinningAdapters) {
        adapter->ExtendTimeSamples(
            interval,
            &skelTimesMap[adapter->GetSkelAdapter()]);
    }

    // Each times array may now hold duplicate entries. 
    // Sort and remove dupes from each array.
    WorkParallelForN(
        skelAdapters.size(),
        [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                std::vector<double>& times = skelTimesMap[skelAdapters[i]];
                std::sort(times.begin(), times.end());
                times.erase(std::unique(times.begin(), times.end()),
                            times.end());
            }
        });

    // XXX: Skinning meshes are baked at each time sample at which joint
    // transforms or blend shapes are authored. If the joint transforms
    // are authored at sparse time samples, then the deformed meshes will
    // be linearly interpolated on sub-frames. But linearly interpolating
    // deformed meshes is not equivalent to linearly interpolating the
    // the driving animation, particularly when considering joint rotations.
    // It is impossible to get a perfect match at every possible sub-frame,
    // since the resulting stage may be read at arbitrary sub-frames, but
    // we can at least make sure that the samples are correct at the
    // frames on which the stage is expected to be sampled, based on the
    // stage's time-code metadata.
    // In other words, we wish to bake skinning at every time ordinate at
    /// which the output is expected to be sampled.
    const std::vector<double> stageTimes =
        _GetStagePlaybackTimeCodesInRange(stage, interval);

    // Compute the total union of all time samples.
    std::vector<double> allTimes;
    std::vector<double> tmpUnionTimes;
    _UnionTimes(stageTimes, &allTimes, &tmpUnionTimes);
    for (const auto& pair : skelTimesMap) {
        _UnionTimes(pair.second, &allTimes, &tmpUnionTimes);
    }

    // Actual time samples will be default time + the times above.
    times.clear();
    times.reserve(allTimes.size() + 1);
    times.push_back(UsdTimeCode::Default());
    times.insert(times.end(), allTimes.begin(), allTimes.end());

    // For each skinning adapter, store a bit mask identitying which
    // of the above times should be sampled for the adapter.
    WorkParallelForN(
        skelAdapters.size(),
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                std::vector<bool> timeSampleMask(times.size(), false);

                const auto& timesForSkel = skelTimesMap[skelAdapters[i]];
                if (timesForSkel.empty()) {
                    // Skel has no time samples; only need to
                    // sample at defaults (index=0).
                    timeSampleMask[0] = true;
                } else {
                    for (const double t : timesForSkel) {
                        const auto it =
                            std::lower_bound(allTimes.begin(),
                                             allTimes.end(), t);
                        TF_DEV_AXIOM(it != allTimes.end() && *it == t);
                        // +1 to account for default time (index=0)
                        const size_t index =
                            std::distance(allTimes.begin(), it) + 1;
                        timeSampleMask[index] = true;
                    }
                    if (timesForSkel.size() > 1) {
                        // Mix in any times corresponding to stage playback
                        // that lie within the range of the times for this
                        // skel.
                        const auto start = 
                            std::lower_bound(stageTimes.begin(),
                                             stageTimes.end(),
                                             timesForSkel.front());
                        const auto end =
                            std::upper_bound(stageTimes.begin(),
                                             stageTimes.end(),
                                             timesForSkel.back());

                        for (auto it = start; it != end; ++it) {
                            const auto allTimesIt =
                                std::lower_bound(allTimes.begin(),
                                                 allTimes.end(), *it);
                            TF_DEV_AXIOM(allTimesIt != allTimes.end() &&
                                         *allTimesIt == *it);
                            // +1 to account for default time (index=0)
                            const size_t index =
                                std::distance(allTimes.begin(), allTimesIt) + 1;
                            timeSampleMask[index] = true;
                        }
                    }
                }
                skelAdapters[i]->SetTimeSampleMask(std::move(timeSampleMask));
            }
        });

    return times;
}


/// Convert all SkelRoot prims to Xform prims.
/// This disables the effect of skels, resulting in a normal geometry hierarchy.
void
_ConvertSkelRootsToXforms(const UsdSkelBakeSkinningParms& parms)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Convert SkelRoot prims to Xforms\n");

    // Find the SkelRoot above each binding.
    std::vector<UsdSkelRoot> skelRootsPerBinding(parms.bindings.size());
    for (size_t i = 0; i< parms.bindings.size(); ++i) {
        const unsigned layerIndex = parms.layerIndices[i];
        if (layerIndex < parms.layers.size()) {
            const SdfLayerHandle& layer = parms.layers[layerIndex];
            const UsdSkelBinding& binding = parms.bindings[i];

            for (const auto& skinningQuery : binding.GetSkinningTargets()) {
                if (const UsdSkelRoot root =
                    UsdSkelRoot::Find(skinningQuery.GetPrim())) {

                    if (const SdfPrimSpecHandle spec =
                        SdfCreatePrimInLayer(layer, root.GetPrim().GetPath())) {
                        spec->SetTypeName(_tokens->Xform);
                        spec->SetSpecifier(SdfSpecifierDef);
                    }
                    break;
                }
            }
        }
    }
}


/// Update extents of any prims whose points were modified by skinning,
/// but which weren't directly updated by the main skinning loop.
void
_PostUpdateExtents(
    const std::vector<_SkinningAdapterRefPtr>& skinningAdapters,
    const std::vector<UsdTimeCode>& times)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Post-updating prim extents\n");

    // Idenfity adapters to update.
    std::vector<_SkinningAdapterRefPtr> adaptersToUpdate;
    adaptersToUpdate.reserve(skinningAdapters.size());
    for (const auto& adapter : skinningAdapters) {
        if (adapter->RequiresPostExtentUpdate()) {
            adaptersToUpdate.push_back(adapter);
        }
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Updating extents for %zu prims\n",
        adaptersToUpdate.size());

    if (adaptersToUpdate.empty()) {
        return;
    }

    // Compute all extents.
    std::vector<VtVec3fArray> extents(times.size()*adaptersToUpdate.size());
    WorkParallelForN(
        adaptersToUpdate.size(),
        [&adaptersToUpdate,&times,&extents](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                const auto& adapter = adaptersToUpdate[i];
                const UsdGeomBoundable boundable(adapter->GetPrim());

                for (size_t ti = 0; ti < times.size(); ++ti) {
                    if (adapter->ShouldProcessAtTime(ti)) {
                        const size_t extentIndex = times.size()*i + ti;
                        UsdGeomBoundable::ComputeExtentFromPlugins(
                            boundable, times[ti], &extents[extentIndex]);
                    }
                }
            }
        });

    // Author the results.
    for (size_t i = 0; i < adaptersToUpdate.size(); ++i) {
        const auto& adapter = adaptersToUpdate[i];
        const UsdGeomBoundable boundable(adapter->GetPrim());
            
        const UsdAttribute extentAttr = boundable.GetExtentAttr();
        extentAttr.Clear();

        for (size_t ti = 0; ti < times.size(); ++ti) {
            const size_t extentIndex = times.size()*i + ti;
            if (!extents[extentIndex].empty()) {
                extentAttr.Set(extents[extentIndex], times[ti]);
            }
        }
    }
}


struct _HashComparePrim
{
    bool    operator()(const UsdPrim& a, const UsdPrim& b) const
            { return a == b; }
    size_t  operator()(const UsdPrim& prim) const
            { return hash_value(prim); }
};


/// Update extents hints of any ancestor models of skinned prims
/// that already define an extents hint.
void
_UpdateExtentHints(
    const std::vector<_SkinningAdapterRefPtr>& skinningAdapters,
    const std::vector<UsdTimeCode>& times)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Updating extent hints\n");

    // Find the models that have extentsHints that need to be updated.
    std::unordered_map<UsdPrim, VtArray<_SkinningAdapterRefPtr>,
                       _HashComparePrim>
        modelToAdaptersMap;

    for (const auto& adapter : skinningAdapters) {
        for (UsdPrim p = adapter->GetPrim();
             !p.IsPseudoRoot(); p = p.GetParent()) {
            if (p.IsModel()) {
                const UsdGeomModelAPI model(p);
                if (auto attr = model.GetExtentsHintAttr()) {
                    modelToAdaptersMap[p].push_back(adapter);
                    // Clear any existing time samples, incase they
                    // include samples that differ from our sampling times.
                    attr.Clear();
                }
            }
        }
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning]   Updating extent hints for %zu prims\n",
        modelToAdaptersMap.size());

    if (modelToAdaptersMap.empty()) {
        return;
    }
    
    // Flatten the map down to vectors, for threading convenience.
    std::vector<UsdPrim> modelsToUpdate;
    modelsToUpdate.resize(modelToAdaptersMap.size());
    std::vector<VtArray<_SkinningAdapterRefPtr>> adaptersPerModel;
    adaptersPerModel.resize(modelToAdaptersMap.size());
    {
        size_t index = 0;
        for (const auto& pair : modelToAdaptersMap) {
            modelsToUpdate[index] = pair.first;
            adaptersPerModel[index] = pair.second;
            ++index;
        }
    }

    // Compute all extent hints.
    std::vector<VtVec3fArray> extents(times.size()*skinningAdapters.size());
    WorkParallelForN(
        times.size(),
        [&](size_t start, size_t end)
        {
            UsdGeomBBoxCache bboxCache(
                times[start],
                UsdGeomImageable::GetOrderedPurposeTokens(),
                /*useExtentsHint*/ false);

            for (size_t ti = start; ti < end; ++ti) {

                const UsdTimeCode time = times[ti];

                bboxCache.SetTime(time);

                for (size_t i = 0; i < adaptersPerModel.size(); ++i) {
                    
                    bool shouldProcess = false;
                    // Make sure the adapter arrays are accessed via
                    // const refs to avoid detaching.
                    const VtArray<_SkinningAdapterRefPtr>& adapters =
                        adaptersPerModel[i];
                    for (const auto& adapter : adapters) {
                        if (adapter->ShouldProcessAtTime(ti)) {
                            shouldProcess = true;
                            break;
                        }
                    }
                    if (shouldProcess) {
                        const UsdGeomModelAPI model(modelsToUpdate[i]);
                        const size_t extentIndex = times.size()*i + ti;
                        extents[extentIndex] =
                            model.ComputeExtentsHint(bboxCache);
                    }
                }
            }
        });

    // Author the results.
    for (size_t i = 0; i < modelsToUpdate.size(); ++i) {
        const UsdGeomModelAPI model(modelsToUpdate[i]);
        for (size_t ti = 0; ti < times.size(); ++ti) {
            const size_t extentIndex = times.size()*i + ti;
            if (!extents[extentIndex].empty()) {
                model.SetExtentsHint(extents[extentIndex], times[ti]);
            }
        }
    }
}


bool
_SaveLayers(const UsdSkelBakeSkinningParms& parms)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Saving %zu layers\n",
        parms.layers.size());

    std::atomic_bool error(false);
    WorkParallelForEach(
        parms.layers.begin(), parms.layers.end(),
        [&error](const SdfLayerHandle& layer) {
            if (!layer->Save()) {
                error = true;
            }
        });
    return !error;
}


} // namespace


bool
UsdSkelBakeSkinning(const UsdSkelCache& skelCache,    
                    const UsdSkelBakeSkinningParms& parms,
                    const GfInterval& interval)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Baking skinning over range %s "
        "for %zu bindings in %zu layers.\n"
        "Deformation flags:\n%s",
        TfStringify(interval).c_str(),
        parms.bindings.size(), parms.layers.size(),
        _DeformationFlagsToString(parms.deformationFlags, "    ").c_str());

    UsdGeomXformCache xfCache;

    // Get the stage from the first valid binding.
    UsdStagePtr stage;
    for (const auto& binding : parms.bindings) {
        if (binding.GetSkeleton()) {
            stage = binding.GetSkeleton().GetPrim().GetStage();
            break;
        }
    }
    if (!stage) {
        return false;
    }

    // Create adapters to wrangle IO on skels and skinnable prims.
    std::vector<_SkelAdapterRefPtr> skelAdapters;
    std::vector<_SkinningAdapterRefPtr> skinningAdapters;
    if (!_CreateAdapters(parms, skelCache, &skelAdapters,
                         &skinningAdapters, &xfCache)) {
        return false;
    }

    // Compute an array of times at which the adapters should be processed.
    // The skel adapters are additionally annotated with a mask indicating
    // whether or not each individual skel needs to be processed at each time.
    const std::vector<UsdTimeCode> times =
        _ComputeTimeSamples(stage, interval, skelAdapters,
                            skinningAdapters, &xfCache);

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Baking skinning for %zu prims, "
        "deformed by %zu skels, over %zu time samples\n",
        skinningAdapters.size(), skelAdapters.size(), times.size());

    // Defer change processing.
    std::unique_ptr<SdfChangeBlock> changeBlock(new SdfChangeBlock);
    {
        size_t bytesStored = 0;
        std::vector<size_t> bytesStoredPerLayer(parms.layers.size());

        for (size_t ti = 0; ti < times.size(); ++ti) {

            const UsdTimeCode time = times[ti];

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Baking skinning at sample "
                "%zu (time=%s)\n", ti, TfStringify(time).c_str());

            // Update all required transforms for this time.
            xfCache.SetTime(time);

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Reading skel transforms at time %s\n",
                TfStringify(time).c_str());

            for (const auto& skelAdapter : skelAdapters) {
                skelAdapter->UpdateTransform(ti, &xfCache);
            }

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Reading skinned prim transforms "
                "at time %s\n", TfStringify(time).c_str());

            for (const auto& skinningAdapter : skinningAdapters) {
                skinningAdapter->UpdateTransform(ti, &xfCache);
            }

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Reading skel animation at time %s\n",
                TfStringify(time).c_str());

            // Update all skel animations for this time.
            WorkParallelForEach(
                skelAdapters.begin(), skelAdapters.end(),
                [time,ti](const _SkelAdapterRefPtr& skelAdapter) {
                    skelAdapter->UpdateAnimation(time, ti);
                });

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Updating skinning adapters at time %s\n",
                TfStringify(time).c_str());

            // Update all skinning adapters for this time.
            WorkParallelForEach(
                skinningAdapters.begin(), skinningAdapters.end(),
                [time,ti](
                    const _SkinningAdapterRefPtr& skinningAdapter) {  
                    skinningAdapter->Update(time, ti);
                });

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Writing data to layers at time %s\n",
                TfStringify(time).c_str());

            // Write the results from each skinning adapter.
            // This must be done in serial for each layer being written,
            // but different layers may be written to at the same time.
            WorkParallelForN(
                parms.layers.size(),
                [time, ti, &parms, &skinningAdapters, &bytesStoredPerLayer]
                (size_t start, size_t end)
                {
                    for (size_t i = start; i < end; ++i) {
                        size_t bytesStored = 0;
                        for (const auto& adapter : skinningAdapters) {
                            if (adapter->GetLayerIndex() == i) {
                                bytesStored += adapter->Write(time, ti);
                            }
                        }
                        bytesStoredPerLayer[i] = bytesStored;
                    }
                });
            bytesStored += std::accumulate(bytesStoredPerLayer.begin(),
                                           bytesStoredPerLayer.end(), 0);

            if (parms.memoryLimit && parms.saveLayers &&
                bytesStored > parms.memoryLimit) {
                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning] Memory limit exceeded "
                    "(%zu bytes of pending data). Flushing data to disk.\n",
                    bytesStored);

                // The values stored in Sdf have exceeded our memory limits.
                // Save the layer to flush changes to disk.
                if (!_SaveLayers(parms)) {
                    return false;
                }

                bytesStored = 0;
            }
        }

        _ConvertSkelRootsToXforms(parms);
    }

    // Expire the change block. Changes will be processed on the stage.
    {
        TRACE_SCOPE("Process stage changes");

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning] Processing stage changes\n");

        changeBlock.reset();
    }

    if (parms.updateExtents) {
        _PostUpdateExtents(skinningAdapters, times);
    }
    if (parms.updateExtentHints) {
        _UpdateExtentHints(skinningAdapters, times);
    }
    return !parms.saveLayers || _SaveLayers(parms);
}


bool
UsdSkelBakeSkinning(const UsdPrimRange& range, const GfInterval& interval)
{
    UsdSkelBakeSkinningParms parms;
    // Backwards-compatibility: do not save during skinning.
    parms.saveLayers = false;

    UsdSkelCache skelCache;

    std::vector<UsdSkelBinding> bindings;

    // Build up the complete list of bindings to process.
    for (auto it = range.begin(); it != range.end(); ++it) {
        if (it->IsA<UsdSkelRoot>()) {

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Populating cache for <%s>\n",
                it->GetPath().GetText());

            const UsdSkelRoot skelRoot(*it);
            skelCache.Populate(skelRoot);

            if (skelCache.ComputeSkelBindings(skelRoot, &bindings)) {
                parms.bindings.insert(parms.bindings.end(),
                                      bindings.begin(), bindings.end());
            }
        }
    }
    if (parms.bindings.empty()) {
        // Nothing to do.
        return true;
    }

    // Follow the first binding to determine the stage
    // (and from there, the current authoring layer)
    parms.layers.assign(
        1, parms.bindings[0].GetSkeleton().GetPrim().GetStage()
        ->GetEditTarget().GetLayer());
    parms.layerIndices.assign(parms.bindings.size(), 0);
    return UsdSkelBakeSkinning(skelCache, parms, interval);
}


bool
UsdSkelBakeSkinning(const UsdSkelRoot& skelRoot, const GfInterval& interval)
{
    UsdSkelBakeSkinningParms parms;
    // Backwards-compatibility: do not save during skinning.
    parms.saveLayers = false;

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Populating cache for <%s>\n",
        skelRoot.GetPrim().GetPath().GetText());

    UsdSkelCache skelCache;
    skelCache.Populate(skelRoot);

    if (skelCache.ComputeSkelBindings(skelRoot, &parms.bindings)) {
        if (parms.bindings.empty()) {
            // Nothing to do.
            return true;
        }
        parms.layers.assign(
            1, skelRoot.GetPrim().GetStage()->GetEditTarget().GetLayer());
        parms.layerIndices.assign(parms.bindings.size(), 0);
        return UsdSkelBakeSkinning(skelCache, parms, interval);
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
