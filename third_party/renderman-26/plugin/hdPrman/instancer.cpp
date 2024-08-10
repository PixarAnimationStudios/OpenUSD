//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/instancer.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/light.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/visitValue.h"
#include "pxr/base/work/loops.h"

#include "pxr/pxr.h"

#include <prmanapi.h>
#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>
#include <stats/Roz.h>

#include <tbb/spin_rw_mutex.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#if _PRMANAPI_VERSION_ >= 27 // Check for prman batch API
#define PRMAN_HAS_BATCH
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_PRMAN_DISABLE_NESTED_INSTANCING, false,
    "disable riley nested instancing in hdprman");

const int HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH = 4;

// **********************************************
// **        Internal helper functions         **
// **********************************************
namespace {

template <typename T1, typename T2, unsigned int C>
void _AccumulateSampleTimes(
    const HdTimeSampleArray<T1,C>& in,
    HdTimeSampleArray<T2,C>& out)
{
    // XXX: This is just a straight copy that works fine in situations where
    // out's sample range is within in's. But if out's sample range begins
    // before in's (out.times[0] < in.times[0]) or ends after in's
    // (out.times[-1] > in.times[-1]), we're gonna lose part of the range.
    if (in.count > out.count) {
        out.Resize(in.count);
        out.times = in.times;
    }
}

// Visitor for VtVisitValue; will retrieve the value at the specified
// index as a VtValue when the visited value is array-typed. Returns
// empty VtValue when the visited value is not array-typed, or when the
// index points beyond the end of the array.
struct _GetValueAtIndex {
    _GetValueAtIndex(const size_t index) : _index(index) { }
    template <class T>
    const VtValue operator()(const VtArray<T>& array) const
    {
        if (array.size() > _index) {
            return VtValue(array[_index]);
        }
        return VtValue();
    }
    const VtValue operator()(const VtValue& val) const
    {
        return VtValue();
    }
private:
    size_t _index;
};

template <typename M>
M _ConvertMatrix(GfMatrix4d&& src);

template<>
GfMatrix4d _ConvertMatrix<GfMatrix4d>(GfMatrix4d&& src) { return src; }

template<>
RtMatrix4x4 _ConvertMatrix<RtMatrix4x4>(GfMatrix4d&& src) { 
    return HdPrman_Utils::GfMatrixToRtMatrix(src);
}

template <typename M, unsigned int C>
HdTimeSampleArray<M,C> _MultiplyTransforms(
    const HdTimeSampleArray<GfMatrix4d,C>& lhs,
    const HdTimeSampleArray<GfMatrix4d,C>& rhs)
{
    HdTimeSampleArray<M,C> dest;
    _AccumulateSampleTimes(lhs, dest);
    _AccumulateSampleTimes(rhs, dest);
    if (lhs.count == 0 ||
        (lhs.count == 1 && lhs.values[0] == GfMatrix4d(1))) {
        for (size_t j = 0; j < dest.count; ++j) {
            dest.values[j] = _ConvertMatrix<M>(rhs.Resample(dest.times[j]));
        }
    } else if (rhs.count == 0 ||
        (rhs.count == 1 && rhs.values[0] == GfMatrix4d(1))) {
        for (size_t j = 0; j < dest.count; ++j) {
            dest.values[j] = _ConvertMatrix<M>(lhs.Resample(dest.times[j]));
        }
    } else {
        for (size_t j = 0; j < dest.count; ++j) {
            dest.values[j] = _ConvertMatrix<M>(
                lhs.Resample(dest.times[j]) * rhs.Resample(dest.times[j]));
        }
    }
    return dest;
}

void _BuildStatsId(
    const SdfPath& instancerId, 
    const size_t index, 
    const SdfPath& protoId, 
    RtParamList& params)
{
    // The stats id is a human readable unique identifier in the form:
    //   </path/to/instancer>[instanceIndex]{prototypeName}
    // It is used for diagnostic purposes to identify instances in a
    // Disgust log. It is somewhat costly to construct, so we only do so
    // when generating a Disgust log.
    static const bool disabled = TfGetenv("RILEY_CAPTURE").empty();
    if (disabled) { return; }
    RtUString val;
    if (params.HasParam(RixStr.k_stats_identifier)) {
        params.GetString(RixStr.k_stats_identifier, val);
        std::string valStr(val.CStr());
        valStr = TfStringReplace(
            valStr, 
            instancerId.GetString(), 
            TfStringPrintf("%s[%zu]", instancerId.GetText(), index));
        val = RtUString(valStr.c_str());
    } else {
        std::string valStr = TfStringPrintf(
            "%s[%zu]{%s}", 
            instancerId.GetText(), 
            index, 
            protoId.GetName().c_str());
        val = RtUString(valStr.c_str());
    }
    params.SetString(RixStr.k_stats_identifier, val);
}

RtUString
_FixupParamName(const TfToken& name) {
    // Instance params with the "ri:attributes:" and "primvars:ri:attributes:"
    // prefixes correspond to renderman-namespace attributes and have that
    // prefix stripped. All other params are in the "user:" namespace, so if
    // they don't have that prefix we need to add it.
    static const std::string userPrefix("user:");
    static const std::string riAttrPrefix("ri:attributes:");
    static const std::string primvarsRiAttrPrefix("primvars:ri:attributes:");
    RtUString rtname;
    if (TfStringStartsWith(name.GetString(), userPrefix)) {
        rtname = RtUString(name.GetText());
    } else if (TfStringStartsWith(name.GetString(), riAttrPrefix)) {
        rtname = RtUString(name.GetString()
            .substr(riAttrPrefix.length()).c_str());
    } else if (TfStringStartsWith(name.GetString(), primvarsRiAttrPrefix)) {
        rtname = RtUString(name.GetString()
            .substr(primvarsRiAttrPrefix.length()).c_str());
    } else {
        rtname = RtUString(TfStringPrintf("user:%s", name.GetText()).c_str());
    }
    return rtname;
}

// Save one code indentation level when we don't have anything to
// amortize across a batch.
template <typename Fn>
void
_ParallelFor(size_t n, Fn &&cb, size_t grainSize = 1) {
    return WorkParallelForN(n, [&cb](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            std::forward<Fn>(cb)(i);
        }
    }, grainSize);
}

} // anonymous namespace

// **********************************************
// **              Public methods              **
// **********************************************

HdPrmanInstancer::HdPrmanInstancer(
    HdSceneDelegate* delegate,
    SdfPath const& id)
    : HdInstancer(delegate, id)
{ }

HdPrmanInstancer::~HdPrmanInstancer() = default;

HdDirtyBits 
HdPrmanInstancer::GetInitialDirtyBitsMask() const
{
    static const HdDirtyBits dirtyBits =
        HdInstancer::GetInitialDirtyBitsMask() |
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyCategories;
    return dirtyBits;
};

void
HdPrmanInstancer::Sync(
    HdSceneDelegate* delegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    const SdfPath& id = GetId();

    if (TfDebug::IsEnabled(HDPRMAN_INSTANCERS)) {
        using namespace HdPrmanDebugUtil;

        const std::string clr = GetCallerAsString(TF_CALL_CONTEXT);
        const std::string dbs = HdChangeTracker::StringifyDirtyBits(*dirtyBits);
        const std::string pro = SdfPathVecToString(delegate->
            GetInstancerPrototypes(id));
        std::string dps;
        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            for (HdInterpolation i = HdInterpolationConstant;
                i != HdInterpolationCount; i = HdInterpolation(i+1)){
                for (const HdPrimvarDescriptor& primvar : 
                    delegate->GetPrimvarDescriptors(id, i)) {
                    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, 
                        primvar.name)) {
                        if (dps.empty()) {
                            dps += "    dirty primvars    : ";
                        } else {
                            dps += "                      : ";
                        }
                        const VtValue& val = delegate->Get(id, primvar.name);
                        dps += TfStringPrintf("(%s) (%s) %s\n",
                            TfEnum::GetName(i).c_str(), 
                            val.GetTypeName().c_str(),
                            primvar.name.GetText());
                    }
                }
            }
        }

        std::string msg;
        msg += TfStringPrintf("*** Sync called on <%s>\n", id.GetText());
        msg += TfStringPrintf("    dirtyBits         : %s\n", dbs.c_str());
        if (!dps.empty()) {
            msg += dps;
        }
        msg += TfStringPrintf("    prototypes        : [%s]\n", pro.c_str());
        msg += TfStringPrintf("    caller            : %s\n", clr.c_str());
        TF_DEBUG(HDPRMAN_INSTANCERS).Msg("%s\n", msg.c_str());
    }

    _UpdateInstancer(delegate, dirtyBits);

    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);

    // Convert (and cache) instancer coordinate systems.
    if (HdPrman_RenderParam::RileyCoordSysIdVecRefPtr convertedCoordSys =
        param->ConvertAndRetainCoordSysBindings(delegate, id)) {
        _coordSysList.count = convertedCoordSys->size();
        _coordSysList.ids = convertedCoordSys->data();
    }

    // cache instance primvars
    _SyncPrimvars(dirtyBits);

    // cache the instancer and instance transforms
    _SyncTransforms(dirtyBits, param);
    
    // cache the instancer and instance categories
    _SyncCategories(dirtyBits);

    // cache the instancer visibility
    _SyncVisibility(dirtyBits);

    // If anything has changed, internally flag all previously-populated
    // instances as dirty. Since instances are grouped by prototype prim id
    // and Populate gets called one prototype prim at a time, we set a dirty
    // flag for each known prototype prim id. Each gets cleared once Populate
    // has been called with the corresponding prototype prim. This helps avoid
    // unnecessary updates to riley instances in Populate.
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id) ||
        HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
        *dirtyBits & HdChangeTracker::DirtyCategories ||
        HdChangeTracker::IsVisibilityDirty(*dirtyBits, id) ||
        HdChangeTracker::IsInstanceIndexDirty(*dirtyBits, id)) {
        _SetPrototypesDirty();
    }
}

void
HdPrmanInstancer::Finalize(HdRenderParam *renderParam)
{
    HD_TRACE_FUNCTION();
    TF_DEBUG(HDPRMAN_INSTANCERS).Msg("*** Finalize called on <%s>\n\n",
        GetId().GetText());
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley *riley = param->AcquireRiley();

    // Release retained conversions of coordSys bindings.
    param->ReleaseCoordSysBindings(GetId());
    
    // Delete all my riley instances
    _protoMap.citerate([riley](const SdfPath& /* path */,
        const _ProtoMapEntry& entry) {
        for (const auto& rp : entry.map) {
            const _InstanceIdVec& ids = rp.second;
            _ParallelFor(ids.size(), [&](size_t i) {
                const _RileyInstanceId& ri = ids[i];
                if (ri.lightInstanceId != riley::LightInstanceId::InvalidId()) {
                    riley->DeleteLightInstance(ri.groupId, ri.lightInstanceId);
                } else if (ri.geoInstanceId !=
                    riley::GeometryInstanceId::InvalidId()) {
                    riley->DeleteGeometryInstance(ri.groupId, ri.geoInstanceId);
                }
            });
        }
    });

    // Clear my proto map
    _protoMap.clear();

    // Depopulate instances of my groups
    HdPrmanInstancer* parent = _GetParentInstancer();
    if (parent) {
        parent->Depopulate(renderParam, GetId());
    }

    // Delete my prototype groups
    _groupMap.citerate([&](const _FlattenData& /* fd */,
        const riley::GeometryPrototypeId& gp) {
        if (gp != riley::GeometryPrototypeId::InvalidId()) {
            riley->DeleteGeometryPrototype(gp);
        }
    });
    
    // Clear my group map
    _groupMap.clear();
    _groupCounters.clear();
}

void HdPrmanInstancer::Populate(
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const SdfPath& hydraPrototypeId,
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
    const riley::CoordinateSystemList& coordSysList,
    const RtParamList& prototypeParams,
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>&
        prototypeXform,
    const std::vector<riley::MaterialId>& rileyMaterialIds,
    const SdfPathVector& prototypePaths,
    const riley::LightShaderId& lightShaderId)
{
    // This public Populate signature does not accept the last two arguments
    // that the private _PopulateInstances does; those are only available to 
    // HdPrmanInstancer. This lets us keep their messy types private.
    
    _PopulateInstances(
        renderParam,
        dirtyBits,
        hydraPrototypeId,
        hydraPrototypeId,
        rileyPrototypeIds,
        coordSysList,
        prototypeParams,
        prototypeXform,
        rileyMaterialIds,
        prototypePaths,
        lightShaderId,
        { },
        { });
}

void HdPrmanInstancer::Depopulate(
    HdRenderParam* renderParam,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& excludedPrototypeIds)
{
    HD_TRACE_FUNCTION();
    if (TfDebug::IsEnabled(HDPRMAN_INSTANCERS)) {
        TF_DEBUG(HDPRMAN_INSTANCERS).Msg(
            "*** Depopulate called on Instancer <%s>\n"
            "    prototypePrimPath : <%s>\n"
            "    excludedIds       : [%s]\n"
            "    caller            : %s\n\n",
            GetId().GetText(), prototypePrimPath.GetText(),
            HdPrmanDebugUtil::RileyIdVecToString(excludedPrototypeIds).c_str(),
            HdPrmanDebugUtil::GetCallerAsString(TF_CALL_CONTEXT).c_str());
    }
    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();

    _RemoveDeadInstances(riley, prototypePrimPath, excludedPrototypeIds);
    _CleanDisusedGroupIds(param);
}

// **********************************************
// **    Private methods called during Sync    **
// **********************************************

void HdPrmanInstancer::_SyncPrimvars(const HdDirtyBits* dirtyBits)
{

    // This method syncs USD primvars authored on the instancer into a cache.
    // This cache will later be used to compose riley instance params from the
    // USD primvars. Under Hydra 1.0, only instance-rate USD primvars are
    // available, and any authored as "varying", "vertex", or "faceVarying"
    // have already been converted to "instance". However, in Hydra 2.0, all
    // interpolation types are available, and none have been converted from
    // "varying", "vertex", or "faceVarying" to "instance". So we have to query
    // for each interpolation type to be sure to capture all USD primvars that
    // should be applied per-instance.
    //
    // The exclusion here of "constant" and "uniform" USD primvars is an open
    // point of controversy insofar as point instancers are concerned. In
    // theory, those should be inherited by prototypes that are descendants of
    // the point instancer, and we make an explicit attempt to capture those
    // elsewhere. But "constant" and "uniform" primvars on the point instancer
    // cannot be inherited by prototypes that are not also descendants, and in 
    // that case we will not be able to apply them to instances.
    //
    // Some users have requested the ability to use constant primvars authored
    // on the point instancer to control the renderer-specific behavior of the
    // point instancer itself, e.g., by setting a ri-specific param on the
    // point instancer hoping it will be applied to all riley instances the
    // point instancer creates. But such an approach would violate assumptions
    // about inheritance in USD. The issue remains in discussion both internally
    // and in Github issues.

    // XXX: USD Primvars authored on native instances are currently missing
    // under Hydra 2.0 and are not captured here or anywhere else.

    HdSceneDelegate* delegate = GetDelegate();
    SdfPath const& id = GetId();

    // XXX: When finally removing these, eliminate the variables. Replace
    // their usages with the appropriate HdInstancerTokens inline.
#if HD_API_VERSION < 56  // USD_VERSION < 23.05
    TfToken instanceTranslationsToken = HdInstancerTokens->translate;
    TfToken instanceRotationsToken = HdInstancerTokens->rotate;
    TfToken instanceScalesToken = HdInstancerTokens->scale;
    TfToken instanceTransformsToken = HdInstancerTokens->instanceTransform;
#else
    TfToken instanceTranslationsToken = HdInstancerTokens->instanceTranslations;
    TfToken instanceRotationsToken = HdInstancerTokens->instanceRotations;
    TfToken instanceScalesToken = HdInstancerTokens->instanceScales;
    TfToken instanceTransformsToken = HdInstancerTokens->instanceTransforms;
#if HD_API_VERSION < 67  // USD_VERSION < 24.05
    if (TfGetEnvSetting(HD_USE_DEPRECATED_INSTANCER_PRIMVAR_NAMES)) {
        instanceTranslationsToken = HdInstancerTokens->translate;
        instanceRotationsToken = HdInstancerTokens->rotate;
        instanceScalesToken = HdInstancerTokens->scale;
        instanceTransformsToken = HdInstancerTokens->instanceTransform;
    }
#endif
#endif

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        // Get list of USD primvar names for each interp mode and cache each one
        // Solaris allows constant primvars so we need to be able to access them.
        for (HdInterpolation i = HdInterpolationConstant;
            i != HdInterpolationCount; i = HdInterpolation(i+1)) {
            for (const HdPrimvarDescriptor& primvar :
                delegate->GetPrimvarDescriptors(id, i)) {
                // Skip primvars that have special handling elsewhere.
                // The transform primvars are all handled in
                // _SyncTransforms.
                if (primvar.name == instanceTransformsToken ||
                    primvar.name == instanceRotationsToken ||
                    primvar.name == instanceScalesToken ||
                    primvar.name == instanceTranslationsToken) {
                    continue;
                }
                if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
                    VtValue value = delegate->Get(id, primvar.name);
                    if (!value.IsEmpty()) {
                        _PrimvarValue& entry = _primvarMap[primvar.name];
                        entry.desc = primvar;
                        std::swap(entry.value, value);
                    }
                }
            }
        }
    }
}

bool UnboxOrientations(
    HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> const& box,
    HdTimeSampleArray<VtQuatfArray, HDPRMAN_MAX_TIME_SAMPLES> *outRotates)
{
    HdTimeSampleArray<VtQuathArray, HDPRMAN_MAX_TIME_SAMPLES> rotates;
    if (outRotates->UnboxFrom(box)) {
        if (outRotates->count > 0 && outRotates->values[0].size() > 0){
            return true;
        }
    }
    if (rotates.UnboxFrom(box)) {
        // convert to quatf
        outRotates->Resize(rotates.count);
        outRotates->times = rotates.times;
        for (size_t i=0; i < rotates.count; ++i) {
            if (rotates.values[i].size() > 0) {
                VtQuathArray halfs = rotates.values[i];
                VtQuatfArray floats(halfs.cbegin(), halfs.cend());
                outRotates->values[i] = floats;
            }
        }
        return true;
    }   
    return false;
}

template <typename T>
void
_ValidateSamplesTimes(HdTimeSampleArray<T, HDPRMAN_MAX_TIME_SAMPLES>& samples)
{
    for (size_t i = 0; i < samples.count; ++i) {
        if (samples.values[i].size() != samples.values[0].size()) {
            HdTimeSampleArray<T, HDPRMAN_MAX_TIME_SAMPLES> new_samples;
            new_samples.Resize(1);
            new_samples.times[0] = 0.f;
            new_samples.values[0] = samples.Resample(0.f);
            samples = new_samples;
            return;
        }
    }
}

void
HdPrmanInstancer::_SyncTransforms(const HdDirtyBits* dirtyBits,
                                  HdPrman_RenderParam * param)
{
    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    // XXX: When finally removing these, eliminate the variables. Replace
    // their usages with the appropriate HdInstancerTokens inline. Don't forget
    // to reformat the "not ... expected type" warning messages, too!
#if HD_API_VERSION < 56 // USD_VERSION < 23.05
    TfToken instanceTranslationsToken = HdInstancerTokens->translate;
    TfToken instanceRotationsToken = HdInstancerTokens->rotate;
    TfToken instanceScalesToken = HdInstancerTokens->scale;
    TfToken instanceTransformsToken = HdInstancerTokens->instanceTransform;
#else
    TfToken instanceTranslationsToken = HdInstancerTokens->instanceTranslations;
    TfToken instanceRotationsToken = HdInstancerTokens->instanceRotations;
    TfToken instanceScalesToken = HdInstancerTokens->instanceScales;
    TfToken instanceTransformsToken = HdInstancerTokens->instanceTransforms;
#if HD_API_VERSION < 67 // USD_VERSION < 24.05
    if (TfGetEnvSetting(HD_USE_DEPRECATED_INSTANCER_PRIMVAR_NAMES)) {
        instanceTranslationsToken = HdInstancerTokens->translate;
        instanceRotationsToken = HdInstancerTokens->rotate;
        instanceScalesToken = HdInstancerTokens->scale;
        instanceTransformsToken = HdInstancerTokens->instanceTransform;
    }
#endif
#endif

    // Only include this instancer's own transform if it has no parent. When
    // there is a parent instancer, the parent instancer will apply this
    // instancer's transform to the instances it creates of this instancer's
    // prototype groups.
    const bool includeInstancerXform = _Depth() == 0;

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            instanceTransformsToken) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            instanceTranslationsToken) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            instanceRotationsToken) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            instanceScalesToken)) {

        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> instancerXform;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES>
            boxedInstanceXforms;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedTranslates;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedRotates;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedScales;
        if (includeInstancerXform) {
            delegate->SampleInstancerTransform(id,
#if HD_API_VERSION >= 68
                                               param->GetShutterInterval()[0],
                                               param->GetShutterInterval()[1],
#endif
                                               &instancerXform);
        }
        delegate->SamplePrimvar(id, instanceTransformsToken,
#if HD_API_VERSION >= 68
                                param->GetShutterInterval()[0],
                                param->GetShutterInterval()[1],
#endif
                                
                                &boxedInstanceXforms);
        delegate->SamplePrimvar(id, instanceTranslationsToken,
#if HD_API_VERSION >= 68
                                param->GetShutterInterval()[0],
                                param->GetShutterInterval()[1],
#endif
                                &boxedTranslates);
        delegate->SamplePrimvar(id, instanceScalesToken,
#if HD_API_VERSION >= 68
                                param->GetShutterInterval()[0],
                                param->GetShutterInterval()[1],
#endif
                                &boxedScales);
        delegate->SamplePrimvar(id, instanceRotationsToken,
#if HD_API_VERSION >= 68
                                param->GetShutterInterval()[0],
                                param->GetShutterInterval()[1],
#endif
                                &boxedRotates);

        // Unbox samples held as VtValues
        HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES>
            instanceXforms;
        HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> translates;
        HdTimeSampleArray<VtQuatfArray, HDPRMAN_MAX_TIME_SAMPLES> rotates;
        HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> scales;
        if (!instanceXforms.UnboxFrom(boxedInstanceXforms)) {
            TF_WARN("<%s> %s did not have expected type matrix4d[]",
                instanceTransformsToken.GetText(), id.GetText());
        }
        if (!translates.UnboxFrom(boxedTranslates)) {
            TF_WARN("<%s> %s did not have expected type vec3f[]",
                instanceTranslationsToken.GetText(), id.GetText());
        }
        if (!UnboxOrientations(boxedRotates, &rotates)) {
            TF_WARN("<%s> %s did not have expected type quath[] or quatf[]",
                instanceRotationsToken.GetText(), id.GetText());
        }
        if (!scales.UnboxFrom(boxedScales)) {
            TF_WARN("<%s> %s did not have expected type vec3f[]",
                instanceScalesToken.GetText(), id.GetText());
        }

        // Check the number of instances is constant between time samples.
        _ValidateSamplesTimes(instanceXforms);
        _ValidateSamplesTimes(translates);
        _ValidateSamplesTimes(scales);
        _ValidateSamplesTimes(rotates);

        // As a simple resampling strategy, find the input with the max #
        // of samples and use its sample placement.  In practice we expect
        // them to all be the same, i.e. to not require resampling.
        _sa.Resize(0);
        _AccumulateSampleTimes(instancerXform, _sa);
        _AccumulateSampleTimes(instanceXforms, _sa);
        _AccumulateSampleTimes(translates, _sa);
        _AccumulateSampleTimes(scales, _sa);
        _AccumulateSampleTimes(rotates, _sa);

        // Resample inputs and concatenate transformations.
        for (size_t i=0; i < _sa.count; ++i) {
            const float t = _sa.times[i];
            GfMatrix4d xf(1);
            if (instancerXform.count > 0) {
                xf = instancerXform.Resample(t);
            }
            VtMatrix4dArray ixf;
            if (instanceXforms.count > 0) {
                ixf = instanceXforms.Resample(t);
            }
            VtVec3fArray trans;
            if (translates.count > 0) {
                trans = translates.Resample(t);
            }
            VtQuatfArray rot;
            if (rotates.count > 0) {
                rot = rotates.Resample(t);
            }
            VtVec3fArray scale;
            if (scales.count > 0) {
                scale = scales.Resample(t);
            }

            size_t size = std::max({ 
                ixf.size(), 
                trans.size(), 
                rot.size(), 
                scale.size()
            });

            // Concatenate transformations.
            VtMatrix4dArray &ma = _sa.values[i];
            ma.resize(size);
            for (size_t j=0; j < size; ++j) {
                ma[j] = xf;
                if (trans.size() > j) {
                    GfMatrix4d t(1);
                    t.SetTranslate(GfVec3d(trans[j]));
                    ma[j] = t * ma[j];
                }
                if (rot.size() > j) {
                    GfMatrix4d r(1);
                    r.SetRotate(GfQuatd(rot[j]));
                    ma[j] = r * ma[j];
                }
                if (scale.size() > j) {
                    GfMatrix4d s(1);
                    s.SetScale(GfVec3d(scale[j]));
                    ma[j] = s * ma[j];
                }
                if (ixf.size() > j) {
                    ma[j] = ixf[j] * ma[j];
                }
            }
        }
    }
}

void
HdPrmanInstancer::_SyncCategories(const HdDirtyBits* dirtyBits)
{
    // XXX: Instance categories only exist under native instancing, because
    // point instancer instances are not path-addressable. For point instancers,
    // we want the instances to take the categories of the instancer itself.
    // Ideally, this difference would have been smoothed over for us by the
    // scene delegate, and we would get instance categories for either kind of
    // instancing using GetInstanceCategories(). For point instancers, the
    // delegate would give us an appropriately sized vector of identical
    // category lists pulled from the instancer.
    //
    // Unfortunately, GetInstanceCategories() does not handle point instancing
    // this way. It instead returns an empty vector, leaving it to us to notice
    // and call GetCategories() for the instancer ourselves, something we
    // wouldn't otherwise want to do.
    // 
    // Under point instancing, once we've called GetCategories(), we don't
    // bother copying those categories into _instanceCategories, since we don't
    // really know at this point how many instances we will have; we store them
    // separately in the instancer-level flatten data instead.
    //
    // Under point instancing, _instanceCategories will be an empty vector.
    // Therefore, all indexing into _instanceCategories must be bounds-checked!
    //
    // When we *do* have instance categories (as under native instancing), we
    // make a little optimization by finding any categories common to all
    // instances and moving them to the instancer-level flatten data.

    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    if (*dirtyBits & HdChangeTracker::DirtyCategories) {
        _instancerFlat.categories.clear();
        _instanceCategories = delegate->GetInstanceCategories(id);
        if (_instanceCategories.size() == 0) {
            // Point instancing; use instancer's categories
            const VtTokenArray cats = delegate->GetCategories(id);
            _instancerFlat.categories.insert(cats.begin(), cats.end());
        } else {
            // Native instancing; move common categories to instancer
            VtTokenArray intersection;
            for (size_t i = 0; i < _instanceCategories.size(); ++i) {
                VtTokenArray& instCats = _instanceCategories[i];
                // if any instance has no categories there can be no intersection
                if (instCats.size() == 0) {
                    intersection.clear();
                    break;
                }
                std::sort(instCats.begin(), instCats.end());
                VtTokenArray newIntersection;
                if (i == 0) {
                    newIntersection = instCats;
                } else {
                    std::set_intersection(
                        intersection.cbegin(), intersection.cend(),
                        instCats.cbegin(), instCats.cend(),
                        std::back_inserter(newIntersection));
                }
                if (newIntersection.size() == 0) {
                    intersection.clear();
                    break;
                }
                intersection = newIntersection;
            }
            if (intersection.size() > 0) {
                for (auto& instCats : _instanceCategories) {
                     // already sorted above
                    VtTokenArray newCats;
                    std::set_difference(
                        instCats.cbegin(), instCats.cend(),
                        intersection.cbegin(), intersection.cend(),
                        std::back_inserter(newCats));
                    instCats = newCats;
                }
                _instancerFlat.categories.insert(intersection.cbegin(), 
                    intersection.cend());
            }
        }
    }
}

void
HdPrmanInstancer::_SyncVisibility(const HdDirtyBits* dirtyBits)
{
    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _instancerFlat.SetVisibility(delegate->GetVisible(id));
    }
}

void
HdPrmanInstancer::_SetPrototypesDirty()
{
    HdPrmanInstancer* parent = _GetParentInstancer();
    if (parent && _Depth() > HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH) {
        parent->_SetPrototypesDirty();
    } else {
        _protoMap.iterate([&](const SdfPath& /* pp */, _ProtoMapEntry& entry) {
            entry.dirty = true;
        });
    }
}

// **********************************************
// **  Private methods called during Populate  **
// **********************************************

void
HdPrmanInstancer::_PopulateInstancesFromChild(
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const SdfPath& hydraPrototypeId,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
    const riley::CoordinateSystemList& coordSysList,
    const RtParamList& prototypeParams,
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>&
        prototypeXform,
    const std::vector<riley::MaterialId>& rileyMaterialIds,
    const SdfPathVector& prototypePaths,
    const riley::LightShaderId& lightShaderId,
    const std::vector<_InstanceData>& subInstances,
    const std::vector<_FlattenData>& prototypeFlats)
{
    // When a child instancer has multiple prototype prims, that child instancer
    // may call this method simultaneously from different threads. That can lead
    // to duplicate calls from this instancer to create or delete instances of
    // the child instancer's prototype groups. Both of those are problematic
    // when done in parallel. We expect such calls to be identical, so while
    // we are locked we can throw away any additional calls from the same
    // child instancer (identified by prototypePrimPath).
    tbb::spin_rw_mutex& mutex = _childPopulateLocks.get(prototypePrimPath);
    if (mutex.try_lock()) {
        _PopulateInstances(renderParam, dirtyBits, hydraPrototypeId,
            prototypePrimPath, rileyPrototypeIds, coordSysList, prototypeParams,
            prototypeXform, rileyMaterialIds, prototypePaths, lightShaderId,
            subInstances, prototypeFlats);
        _childPopulateLocks.erase(prototypePrimPath);
        mutex.unlock();
    }
}

void
HdPrmanInstancer::_PopulateInstances(
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const SdfPath& hydraPrototypeId,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
    const riley::CoordinateSystemList& coordSysList,
    const RtParamList& prototypeParams,
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>&
        prototypeXform,
    const std::vector<riley::MaterialId>& rileyMaterialIds,
    const SdfPathVector& prototypePaths,
    const riley::LightShaderId& lightShaderId,
    const std::vector<_InstanceData>& subInstances,
    const std::vector<_FlattenData>& prototypeFlats)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

#if defined(PRMAN_HAS_BATCH)
    // RtScopedBatchMemoryTracking
    RtScopedBatchMemoryTracking batchedMemoryTracking(
        RtScopedBatchMemoryTracking::k_batchGeometryInstance);
#endif

    // This is our main workhorse. This method instructs the instancer to
    // generate riley instances of a given prototype. When coming from the
    // public Populate method, that prototype is a hydra prototype whose riley
    // prototypes have already been constructed (see gprim.h). When coming from
    // a child instancer, that prototype will usually be the child instancer
    // itself (along with its associated riley geometry prototype groups).
    // However, if the child instancer is too deep for nested instancing, it
    // will instead provide the same prototype it was given along with a list
    // of InstanceData structures, which contain all the information about the
    // instances the child would have made if it could have. This instancer must
    // then multiply that list by its own instances, and either create them or
    // pass them along to *its* parent if it is also too deep.
    //
    // Further complicating issues, this method may be called concurrently from
    // multiple threads, so some actions must be gated behind mutex locks.

    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();
    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    const SdfPath& instancerId = GetId();
    HdPrmanInstancer* parentInstancer = _GetParentInstancer();
    const int depth = _Depth();
    bool instancesNeedUpdate = _protoMap.get(prototypePrimPath).dirty
        || (*dirtyBits & HdChangeTracker::DirtyPrimvar)
        || (*dirtyBits & HdChangeTracker::DirtyMaterialId)
        || (*dirtyBits & HdChangeTracker::DirtyTransform)
        || (*dirtyBits & HdChangeTracker::DirtyVisibility);
    bool anyGroupIdChanged = false;
    bool isLight = lightShaderId != riley::LightShaderId::InvalidId();

    if (TfDebug::IsEnabled(HDPRMAN_INSTANCERS)) {
        using namespace HdPrmanDebugUtil;

        std::string pid = " -none- ";
        if (parentInstancer) {
            pid = parentInstancer->GetId().GetAsString();
        }

        const VtIntArray instanceIndices = delegate->GetInstanceIndices(
            instancerId, hydraPrototypeId);

        std::string ins = TfStringPrintf("%lu instances ", instanceIndices.size());
        size_t total = instanceIndices.size();
        if (subInstances.size() > 0) {
            ins += TfStringPrintf("of %lu subInstances ", subInstances.size());
            total *= subInstances.size();
        }
        ins += TfStringPrintf("of %lu prototypes ", rileyPrototypeIds.size());
        total *= rileyPrototypeIds.size();
        ins += TfStringPrintf("= %lu Riley instances", total);

        std::string lsi = "- none -";
        if (lightShaderId != riley::LightShaderId::InvalidId()) {
            lsi = TfStringPrintf("(%u)", lightShaderId.AsUInt32());
        }
    
        const HdDirtyBits instDirtyBits = renderIndex.GetChangeTracker()
            .GetInstancerDirtyBits(instancerId);
        const std::string dbs = HdChangeTracker::StringifyDirtyBits(*dirtyBits);
        const std::string idb = HdChangeTracker::StringifyDirtyBits(instDirtyBits);
        const std::string pro = RileyIdVecToString(rileyPrototypeIds);
        const std::string mid = RileyIdVecToString(rileyMaterialIds);
        const std::string pps = SdfPathVecToString(prototypePaths);
        const std::string clr = GetCallerAsString(TF_CALL_CONTEXT);
        const std::string pat = RtParamListToString(prototypeParams, 24);
        const std::string pxf = MatrixToString(prototypeXform.values[0], 24, 3);

        std::string str;
        str += TfStringPrintf("*** Populate called on <%s>\n", instancerId.GetText());
        str += TfStringPrintf("    dirtyBits         : %s\n", dbs.c_str());
        str += TfStringPrintf("    instDirtyBits     : %s\n", idb.c_str());
        str += TfStringPrintf("    hydraPrototypeId  : <%s>\n", hydraPrototypeId.GetText());
        str += TfStringPrintf("    prototypePrimPath : <%s>\n", prototypePrimPath.GetText());
        str += TfStringPrintf("    rileyPrototypeIds : (%s)\n", pro.c_str());
        str += TfStringPrintf("    rileyMaterialIds  : (%s)\n", mid.c_str());
        str += TfStringPrintf("    prototypePaths    : [%s]\n", pps.c_str());
        str += TfStringPrintf("    lightShaderId     : %s\n", lsi.c_str());
        str += TfStringPrintf("    prototypeParams   : %s\n", pat.c_str());
        str += TfStringPrintf("    prototypeXform    : %s\n", pxf.c_str());
        str += TfStringPrintf("    instances         : %lu\n", instanceIndices.size());
        str += TfStringPrintf("    subInstances      : %lu\n", subInstances.size());
        str += TfStringPrintf("    total instances   : %s\n", ins.c_str());
        str += TfStringPrintf("    parentInstancer   : <%s>\n", pid.c_str());
        str += TfStringPrintf("    depth             : %i\n", depth);
        str += TfStringPrintf("    caller            : %s\n", clr.c_str());
        TF_DEBUG(HDPRMAN_INSTANCERS).Msg("%s\n", str.c_str());
    }

    TF_VERIFY(
        rileyMaterialIds.size() == rileyPrototypeIds.size(), 
        "rileyMaterialIds size mismatch: %lu != %lu", 
        rileyMaterialIds.size(), rileyPrototypeIds.size());
    TF_VERIFY(
        prototypePaths.size() == rileyPrototypeIds.size(),
        "prototypePaths size mismatch: %lu != %lu",
        prototypePaths.size(), rileyPrototypeIds.size());
    TF_VERIFY(
        prototypeFlats.size() == 0 || 
        prototypeFlats.size() == rileyPrototypeIds.size(),
        "prototypeFlats size mismatch: %lu != %lu",
        prototypeFlats.size(), rileyPrototypeIds.size());

    // For analytic lights only, rileyPrototypeIds may have only a single,
    // invalid id. In that case, lightData with a valid shader id is required.
    if (rileyPrototypeIds.size() == 1 &&
        rileyPrototypeIds[0] == riley::GeometryPrototypeId::InvalidId()) {
        TF_VERIFY(isLight,
            "When called with a single invalid "
            "prototype id, a light shader id is required");
    }
    
    instancesNeedUpdate = _RemoveDeadInstances(riley, prototypePrimPath,
        rileyPrototypeIds) || instancesNeedUpdate;

    std::vector<_InstanceData> instances;

    // hydraPrototypeId corresponds to the hydra prototype, which might be a
    // child instancer. prototypePrimPath corresponds to the prim represented by
    // the rileyPrototypeIds, which could be a child instancer or prototype
    // geometry. In most cases, they are equal:
    //  1. Both paths are the same, and point to a geometry prim that is one of
    //     this instancer's prototypes and has already been represented in riley
    //     during GPrim::Sync(), resulting in the ids in rileyPrototypeIds which
    //     are all of a non-group primType; or
    //  2. Both paths are the same, and point to a child instancer prim that is
    //     this instancer's prototype and has already been represented in riley
    //     by the child instancer, resulting in the ids in rileyPrototypeIds
    //     which are all of primType "Group".
    // They differ only when the prototype is an instancer and that instancer
    // was too deep to put its riley instances into a group. In that case, 
    // hydraPrototypeId will be the instancer below, while prototypePrimPath 
    // will be the prototype prim path originally given to Populate, because
    // that's the prim represented by the rileyPrototypeIds we've been given.
    // We need the former to retrieve instance indices, params, and
    // transforms for the nested instancer prototype, while we use the latter to
    // track our riley instances and their prototype ids in _protoMap and
    // retrieve USD primvars affecting the prototype.
    
    // We might receive no instances from the too-deep nested instacer, because
    // the nested instancer no longer has any instances of this prototype. In
    // that case, we should not make any instances of this prototype here, and
    // delete any we already have.

    if (hydraPrototypeId == prototypePrimPath || subInstances.size() > 0) {
        _ComposeInstances(hydraPrototypeId, subInstances, instances);
    }

    // TODO: if depth *decreases*, how will no-longer-too-deep-child signal its
    // parent to release the flattened instances?

    // Check for flattening based on env setting or depth. When this instancer
    // is too deeply nested for riley nesting support, or when riley nesting
    // is disabled, we need to pass the bag of composed instances that this
    // instancer would have pushed to riley up to the parent instancer instead.
    // The parent instancer will then duplicate each instance in the bag once
    // for each instance it's expected to generate, effectively multiplying this
    // instancer's instances by its own.
    if (parentInstancer &&
        (TfGetEnvSetting(HD_PRMAN_DISABLE_NESTED_INSTANCING) ||
            depth > HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH)) {

        // TODO: Instancer params?

        // Ensure the protoMap is clear of instances (perhaps depth increased?)
        _ResizeProtoMap(riley, prototypePrimPath, rileyPrototypeIds, 0);

        // Sample this instancer's transform. The instance transforms do not
        // include it. We must multiply the instance transforms by this
        // instancer's transform.
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
        delegate->SampleInstancerTransform(instancerId,
#if HD_API_VERSION >= 68
                                           param->GetShutterInterval()[0],
                                           param->GetShutterInterval()[1],
#endif
                                           &xf);
        
        for (_InstanceData& instance : instances) {
            instance.transform = _MultiplyTransforms<GfMatrix4d>(
                instance.transform, xf);
        }

        // Send allInstances up to the parent to populate
        parentInstancer->_PopulateInstancesFromChild(
            renderParam,
            dirtyBits,
            instancerId,
            prototypePrimPath,
            rileyPrototypeIds,
            coordSysList, 
            prototypeParams,
            prototypeXform,
            rileyMaterialIds,
            prototypePaths,
            lightShaderId,
            instances,
            { }
        );
        return;
    }

    if (instancesNeedUpdate) {
        // Allocate the protoMap; this deletes instances if instances is empty
        _ResizeProtoMap(riley, prototypePrimPath, rileyPrototypeIds, instances.size());
    }

    if (instancesNeedUpdate && instances.size() > 0) {

        // Finalize the prototype-derived params & flats
        std::vector<RtParamList> protoParams;
        std::vector<_FlattenData> protoFlats;
        std::vector<TfToken> protoRenderTags;

        _ComposePrototypeData(
            prototypePrimPath,
            prototypeParams,
            isLight,
            rileyPrototypeIds, 
            prototypePaths,
            prototypeFlats,
            protoParams,
            protoFlats,
            protoRenderTags);
        
        // Prepare each instance to be sent to riley
        _ParallelFor(instances.size(), [&](size_t i) {

#if defined(PRMAN_HAS_BATCH)
            // RtScopedBatchMemoryTracking
            RtScopedBatchMemoryTracking batchMemoryTrackingInner(
                RtScopedBatchMemoryTracking::k_batchGeometryInstance);
#endif

            const _InstanceData& instance = instances[i];

            // Multiply the prototype transform by the instance transform. If
            // this is a top-level instancer with no parent, the instance
            // transform already includes the instancer transform. If this
            // instancer is nested, the instance transform does not include the
            // instancer transform; it will be supplied to the parent
            // instancer separately.
            _RtMatrixSA xform = _MultiplyTransforms<RtMatrix4x4>(
                prototypeXform, instance.transform);

            // Convert the matrix array to riley's Transform type
            const riley::Transform rileyXform = {
                unsigned(xform.count),
                xform.values.data(),
                xform.times.data()
            };

            // If the hydra prototype prim consists of multiple riley
            // prototypes (e.g., in the case of GeomSubsets), we must make 
            // one riley instance for each riley prototype
            for (size_t j = 0; j < rileyPrototypeIds.size(); ++j) {
                // This is expected to be InvalidId for analytic lights
                const riley::GeometryPrototypeId& protoId = 
                    rileyPrototypeIds[j];
                
                riley::MaterialId matId = rileyMaterialIds[j];
                RtParamList params = instance.params; // a copy
                // Merge in params derived from the hydra prototype prim, which
                // are stronger than the instance-derived ones. We use Update
                // so that the prototype-derived params will overwrite the
                // instance-derived ones.
                params.Update(protoParams[j]);
                
                // append subset name to stats:identifier
                if (prototypePaths[j] != hydraPrototypeId) {
                    std::string protoName = TfStringPrintf(
                        "{%s}", prototypePaths[j].GetName().c_str());
                    RtUString sid;
                    if (params.GetString(RixStr.k_stats_identifier, sid)) {
                        std::string sidStr(sid.CStr());
                        if (sidStr.find(protoName) == sidStr.npos) {
                            sid = RtUString(
                                (std::string(sid.CStr()) + protoName).c_str());
                            params.SetString(RixStr.k_stats_identifier, sid);
                        }
                    }
                }

                // compose the final flats
                _FlattenData flats = instance.flattenData; // a copy
                
                // Merge the visibility params derived from the hydra
                // prototype prim. Because these are either present (and set to
                // 0, indicating invisibility) or absent, we use Inherit to
                // obtain the logical OR superset of present params. Inherit
                // also merges the light linking categories.
                flats.Inherit(protoFlats[j]);

                // Acquire the id of the prototype group that will house
                // all instances that share this specific set of flatten data.
                // This group id will be InvalidId when there is no parent
                // instancer.
                riley::GeometryPrototypeId groupId;
                anyGroupIdChanged |= _AcquireGroupId(param, flats, groupId);

                if (!parentInstancer) {
                    // If there is no parent, we can safely set the params
                    // we've been tracking separately in the FlattenData
                    // structure on the instances we're about to make in riley.
                    // We use Update because these should not exist yet.
                    params.Update(flats.params);
                    // And set the appropriate params based on our collected
                    // light linking categories.
                    param->ConvertCategoriesToAttributes(
                        instancerId,
                        { flats.categories.cbegin(), flats.categories.cend() },
                        params);
                }

                // Retrieve the riley instance id
                _RileyInstanceId& instId = _protoMap.get(prototypePrimPath)
                    .map[protoId][i];

                // Check if the instance already exists, and if so, whether
                // it was created in the right prototype group. We can reuse the
                // riley instance id only if the group id and geometry prototype
                // id have not changed. We only need to check the group id;
                // protomap structure guarantees the prototype id is unchanged.
                if (instId.geoInstanceId != riley::GeometryInstanceId::InvalidId()
                    && instId.groupId != groupId) {
                    // the instanceId is valid but the groupId is different
                    riley->DeleteGeometryInstance(
                        instId.groupId, instId.geoInstanceId);
                    _groupCounters.get(instId.groupId)--;
                    instId.geoInstanceId = riley::GeometryInstanceId::InvalidId();
                }
                if (instId.lightInstanceId != riley::LightInstanceId::InvalidId()
                    && instId.groupId != groupId) {
                    // the instanceId is valid but the groupId is different
                    riley->DeleteLightInstance(
                        instId.groupId, instId.lightInstanceId);
                    _groupCounters.get(instId.groupId)--;
                    instId.lightInstanceId = riley::LightInstanceId::InvalidId();
                }

                instId.groupId = groupId;
                _groupCounters.get(instId.groupId)++;

                // Now we branch based on whether we're dealing with lights
                // or geometry

                // XXX: The stats userId will not be unique in the case of
                // nested instancing, but this approach preserves prior
                // behavior in the unnested case.

                const SdfPath instancePath = delegate->GetScenePrimPath(
                    prototypePaths[j], static_cast<int>(i), nullptr);
                riley::UserId userId = riley::UserId(
                    stats::AddDataLocation(instancePath.GetText()).GetValue());

                if (lightShaderId != riley::LightShaderId::InvalidId()) {

                    // XXX: Temporary workaround for RMAN-20704
                    // Destroy the light instance so it will be recreated instead
                    // of being updated, since ModifyLightInstance may crash.

                    if (instId.lightInstanceId != riley::LightInstanceId::InvalidId()) {
                        riley->DeleteLightInstance(
                            instId.groupId, instId.lightInstanceId);
                        instId.lightInstanceId = riley::LightInstanceId::InvalidId();
                    }
                    // XXX: End of RMAN-20704 workaround

                    if (instId.lightInstanceId == riley::LightInstanceId::InvalidId()) {
                        TRACE_SCOPE("riley::CreateLightInstance");
                        instId.lightInstanceId = riley->CreateLightInstance(
                            userId,
                            instId.groupId,
                            protoId,
                            matId,
                            lightShaderId,
                            coordSysList,
                            rileyXform,
                            params);
                    } else if (*dirtyBits) {
                        TRACE_SCOPE("riley::ModifyLightInstance");
                        riley->ModifyLightInstance(
                            instId.groupId,
                            instId.lightInstanceId,
                            &matId,
                            &lightShaderId,
                            &coordSysList,
                            &rileyXform,
                            &params);
                    }
                } else {
                    // Very last thing: prepend renderTag to grouping:membership
                    param->AddRenderTagToGroupingMembership(
                        protoRenderTags[j], params);
                    if (instId.geoInstanceId == riley::GeometryInstanceId::InvalidId()) {
                        TRACE_SCOPE("riley::CreateGeometryInstance");
                        instId.geoInstanceId = riley->CreateGeometryInstance(
                            userId,
                            instId.groupId, 
                            protoId, 
                            matId, 
                            coordSysList, 
                            rileyXform, 
                            params);
                    } else if (*dirtyBits) {
                        TRACE_SCOPE("riley::ModifyGeometryInstance");
                        riley->ModifyGeometryInstance(
                            instId.groupId,
                            instId.geoInstanceId,
                            &matId,
                            &coordSysList,
                            &rileyXform,
                            &params);
                    }
                }
            }
        });

        // We have now fully processed all changes from the last time the
        // instancer was synced down to the riley instances for this particular
        // hydra prototype prim.
        _protoMap.get(prototypePrimPath).dirty = false;
    }

    // clean up disused prototype groups
    anyGroupIdChanged |= _CleanDisusedGroupIds(param);

    if (parentInstancer && (anyGroupIdChanged || 
        HdChangeTracker::IsInstancerDirty(*dirtyBits, instancerId))) {
        // Now we need to tell the parent instancer to make geometry instances
        // of my groups (my groups, my groups, my lovely proto groups).

        // Sample this instancer's transform. The instance transforms did not
        // include it. The parent instancer will instead include it on the
        // instances it makes of this instancer's prototype groups.
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
        delegate->SampleInstancerTransform(instancerId,
#if HD_API_VERSION >= 68
                                           param->GetShutterInterval()[0],
                                           param->GetShutterInterval()[1],
#endif
                                           &xf);

        // Get this instancer's params
        const RtParamList instancerParams = param->ConvertAttributes(
            delegate, instancerId, false);

        // Build the lists of flatten groups and prototype group ids, and 
        // prototype prim paths and material ids, these latter two all being
        // the same for each group.
        std::vector<_FlattenData> flats;
        std::vector<riley::GeometryPrototypeId> ids;
        std::vector<riley::MaterialId> mats;
        SdfPathVector paths;
        _groupMap.citerate([&](const _FlattenData& fd, const riley::GeometryPrototypeId& gp) {
            flats.push_back(fd);
            ids.push_back(gp);
            paths.push_back(instancerId);
            mats.push_back(riley::MaterialId::InvalidId());
        });

        // Populate the parent using _PopulateInstancesFromChild so that we can
        // pass the flatten groups up to it.
        parentInstancer->_PopulateInstancesFromChild(
            renderParam,
            dirtyBits,
            instancerId,
            instancerId,
            ids,
            coordSysList,
            instancerParams,
            xf,
            mats,
            paths,
            // If this prototype was a light and we made light instances in a
            // geometry prototype group, we want the parent instancer to make
            // *geometry* instances of those geometry prototype groups. So we
            // pass the invalid light shader id to it.
            riley::LightShaderId::InvalidId(),
            { },
            flats);
    }
}

void
HdPrmanInstancer::_ComposeInstances(
    const SdfPath& protoId,
    const std::vector<_InstanceData>& subInstances,
    std::vector<_InstanceData>& instances)
{
    HD_TRACE_FUNCTION();
    // XXX: Using riley nested instancing breaks selection. Selection depends on
    // enumerating every instance of a given hydra geometry prototype prim with
    // a unique id and setting that id in riley as identifier:id2. When using
    // riley prototype groups, there is no longer a 1:1 correspondence between
    // hydra instances of a given prototype and riley instances. If instance
    // picking and selection are required, users should disable riley nested
    // instancing by setting HD_PRMAN_DISABLE_NESTED_INSTANCING=1. In future,
    // we may consider adding an instancer id AOV to the picking and selection
    // flow to support precise instance disambiguation.

    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();
    const VtIntArray indices = delegate->GetInstanceIndices(id, protoId);
    instances.clear();
    if (subInstances.empty()) {
        instances.resize(indices.size());
        _ParallelFor(indices.size(), [&](size_t i) {
            const int index = indices[i];
            _InstanceData& instance = instances[i];
            _GetInstanceParams(index, instance.params);
            instance.params.SetInteger(RixStr.k_identifier_id2, int(i));
            _BuildStatsId(id, index, protoId, instance.params);
            _ComposeInstanceFlattenData(index, instance.params, instance.flattenData);
            _GetInstanceTransform(index, instance.transform);
        });
    } else {
        instances.resize(indices.size() * subInstances.size());
        // Iteration order is critical to selection. identifier:id2 must
        // increment in subInstance-major order. So we slow-iterate through
        // this level's instances and fast-iterate through the subInstances.
        // Note: Just to express things as clearly as possible, since this is
        // a parallel loop as written now, it is actually the choice of index
        // for the identifier:id2 parameter and the ordering of the *indexing*
        // that is critical: the parallel for loop is free to set the values
        // in whatever temporal order is most efficient/fastest, as long as the
        // choice we make for the value of identifier:id2 is the same.
        _ParallelFor(indices.size() * subInstances.size(), [&](size_t ii) {
            const size_t i = ii / subInstances.size();
            const size_t index = indices[i];
            const size_t si = ii % subInstances.size();
            const _InstanceData& subInstance  = subInstances[si];
            _InstanceData& instance = instances[ii];
            _GetInstanceParams(index, instance.params);
            instance.params.Update(subInstance.params);
            instance.params.SetInteger(RixStr.k_identifier_id2, int(ii));
            _BuildStatsId(id, index, protoId, instance.params);
            _ComposeInstanceFlattenData(
                index, 
                instance.params, 
                instance.flattenData, 
                subInstance.flattenData);
            _GetInstanceTransform(
                index,
                instance.transform,
                subInstance.transform);
        });
    }
}

void
HdPrmanInstancer::_ComposeInstanceFlattenData(
    const size_t instanceId,
    RtParamList& instanceParams,
    _FlattenData& fd,
    const _FlattenData& fromBelow)
{
    _FlattenData instance;
    if (instanceId < _instanceCategories.size()) {
        instance = _FlattenData(_instanceCategories[instanceId]);
    }

    // Capture fine-grained visibility that may have been authored on the point
    // instancer as instance-varying USD primvars or the native instance; remove
    // these from instanceParams if they exist.
    instance.UpdateVisAndFilterParamList(instanceParams);
    
    fd.Update(_instancerFlat);
    fd.Update(instance);
    fd.Update(fromBelow);
}

bool
HdPrmanInstancer::_RemoveDeadInstances(
    riley::Riley* riley,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& protoIds)
{
    HD_TRACE_FUNCTION();
    // Can't do anything with an empty path
    if (prototypePrimPath.IsEmpty()) { return false; }
    // Check if the protoMap has this path in it first;
    // otherwise the call to get() will insert it.
    if (!_protoMap.has(prototypePrimPath)) { return false; }
    using ProtoMapPair = std::pair<riley::GeometryPrototypeId, _InstanceIdVec>;
    const _ProtoInstMap& protoMap = _protoMap.get(prototypePrimPath).map;
    std::vector<riley::GeometryPrototypeId> oldProtoIds;
    std::transform(
        protoMap.begin(), protoMap.end(),
        std::back_inserter(oldProtoIds),
        [](const ProtoMapPair& pair) { return pair.first; });
    std::vector<riley::GeometryPrototypeId> newProtoIds(
            protoIds.begin(), protoIds.end());

    std::sort(oldProtoIds.begin(), oldProtoIds.end());
    std::sort(newProtoIds.begin(), newProtoIds.end());

    std::vector<riley::GeometryPrototypeId> toRemove;
    std::set_difference(
        oldProtoIds.cbegin(), oldProtoIds.cend(),
        newProtoIds.cbegin(), newProtoIds.cend(),
        std::back_inserter(toRemove));
    if (toRemove.size() > 0) {
        _ResizeProtoMap(riley, prototypePrimPath, toRemove, 0);
    }

    // returns true if there are new geometry prototype ids for this prototype
    std::vector<riley::GeometryPrototypeId> toAdd;
    std::set_difference(
        protoIds.cbegin(), protoIds.cend(),
        oldProtoIds.cbegin(), oldProtoIds.cend(),
        std::back_inserter(toAdd));
    return toAdd.size() > 0;
}

void
HdPrmanInstancer::_ComposePrototypeData(
    const SdfPath& protoPath,
    const RtParamList& globalProtoParams,
    const bool isLight,
    const std::vector<riley::GeometryPrototypeId>& protoIds,
    const SdfPathVector& subProtoPaths,
    const std::vector<_FlattenData>& subProtoFlats,
    std::vector<RtParamList>& protoParams,
    std::vector<_FlattenData>& protoFlats,
    std::vector<TfToken>& protoRenderTags)
{
    HD_TRACE_FUNCTION();
    HdSceneDelegate* delegate = GetDelegate();

    auto SetProtoParams = [&](
        const SdfPath& protoPath,
        RtParamList& params,
        _FlattenData& flats)
    {
        params = globalProtoParams; // copy
        _GetPrototypeParams(protoPath, params);
        const VtTokenArray& cats = delegate->GetCategories(protoPath);
        flats.categories.insert(cats.begin(), cats.end());
        flats.UpdateVisAndFilterParamList(params); // filters out flatten params

        // XXX: Temporary workaround form RMAN-20703
        if (isLight) {
            // Due to limitations in Prman, we currently cannot put light
            // instances and geometry instances in the same prototype group. To
            // force the instancer to separate them, we will make use of
            // _FlattenData::params, the RtParamList we ordinarily just use for
            // flattening visibility params up the instancing hierarchy. For
            // lights, we will set a marker param in the flatten group's
            // param list that will distinguish the flatten group from an
            // otherwise identical one for geometry. The name we use is not
            // important, so long as it has no meaning to riley.
            //
            // We only set this marker param here, and it is never read
            // except when computing the flatten group's hash, when it is
            // picked up by the RtParamList hash functor.
            //
            // See https://jira.pixar.com/browse/RMAN-20703
            flats.params.SetInteger(RtUString("__light"), 1);
        }
        // XXX: End of RMAN-20703 workaround
    };

    // Make at least one set, even when there are no prototype ids,
    // to cover analytic lights.
    const size_t count = std::max(size_t(1), protoIds.size());

    protoParams.resize(count);
    protoFlats.resize(count);
    protoRenderTags.resize(count);

    for (size_t i = 0; i < count; ++i) {
        SetProtoParams(protoPath, protoParams[i], protoFlats[i]);
        if (!isLight) {
            protoRenderTags[i] = delegate->GetRenderTag(protoPath);
        }

        // If prototype is a subset, also get the subset params. While geom
        // subsets should not have USD primvars on them, they may be the targets
        // of light linking and thus have categories to deal with. They may also
        // receive visibility params as part of Hydra's handling of invisible
        // faces, even though visibility cannot be authored on them in USD.
        // XXX: All that is changing in hydra 2, where subsets will be able to
        // have primvars, visibility, and purpose!
        if (i < subProtoPaths.size() && subProtoPaths[i] != protoPath) {
            RtParamList subsetParams;
            _FlattenData subsetFlats;
            SetProtoParams(subProtoPaths[i], subsetParams, subsetFlats);
            protoParams[i].Update(subsetParams);
            protoFlats[i].Update(subsetFlats);
        }

        // Combine any flats received from below for this prototype.
        if (i < subProtoFlats.size()) {
            protoFlats[i].Update(subProtoFlats[i]);
        }
    }
}

void
HdPrmanInstancer::_ResizeProtoMap(
    riley::Riley* riley,
    const SdfPath& prototypePrimPath, 
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds, 
    const size_t newSize)
{
    HD_TRACE_FUNCTION();
    _ProtoInstMap& protoInstMap = _protoMap.get(prototypePrimPath).map;
    for (riley::GeometryPrototypeId protoId : rileyPrototypeIds) {
        _InstanceIdVec& instIdVec = protoInstMap[protoId];
        const size_t oldSize = instIdVec.size();
        if (newSize < oldSize) {
            // XXX: We loop over the range [newSize, oldSize), for a total of
            // (oldSize - newSize) elements.
            _ParallelFor(oldSize - newSize, [&](size_t ii) {
                size_t i = ii + newSize; // Offset the index by 'newSize'
                const _RileyInstanceId& id = instIdVec[i];
                if (id.lightInstanceId != riley::LightInstanceId::InvalidId()) {
                    riley->DeleteLightInstance(id.groupId, id.lightInstanceId);
                    _groupCounters.get(id.groupId)--;
                }
                if (id.geoInstanceId != riley::GeometryInstanceId::InvalidId()) {
                    riley->DeleteGeometryInstance(id.groupId, id.geoInstanceId);
                    _groupCounters.get(id.groupId)--;
                }
            });
        }
        if (oldSize != newSize) {
            instIdVec.resize(newSize);
        }
        if (newSize == 0) {
            protoInstMap.erase(protoId);
        }
    }
    if (protoInstMap.size() == 0) {
        _protoMap.erase(prototypePrimPath);
    }
}

bool
HdPrmanInstancer::_CleanDisusedGroupIds(HdPrman_RenderParam* param)
{
    HD_TRACE_FUNCTION();
    tbb::spin_rw_mutex::scoped_lock lock(_groupIdAcquisitionLock, true);
    riley::Riley* riley = param->AcquireRiley();

    // Gather active and disused
    std::vector<_FlattenData> toDestroy;
    std::unordered_set<riley::GeometryPrototypeId, _ProtoIdHash> active;
    _groupMap.citerate([&](const _FlattenData& fd, const riley::GeometryPrototypeId& gp){
        if (gp != riley::GeometryPrototypeId::InvalidId()) {
            if (_groupCounters.get(gp).load() < 1) {
                toDestroy.push_back(fd);
            } else {
                active.insert(gp);
            }
        }
    });
    // If there are groups to remove and a parent instancer, depopulate the
    // parent preserving the still-active groups
    if (toDestroy.size() > 0) {
        HdPrmanInstancer* parent = _GetParentInstancer();
        if (parent) {
            parent->Depopulate(param, GetId(),
                { active.cbegin(), active.cend() });
        }
    }
    // Destroy the disused prototype groups
    for (const _FlattenData& fd : toDestroy) {
        riley::GeometryPrototypeId& groupId = _groupMap.get(fd);
        if (groupId != riley::GeometryPrototypeId::InvalidId()) {
            riley->DeleteGeometryPrototype(groupId);
            groupId = riley::GeometryPrototypeId::InvalidId();
        }
        _groupMap.erase(fd);
        _groupCounters.erase(groupId);
    }
    // Return true if any were destroyed
    return toDestroy.size() > 0;
}

bool
HdPrmanInstancer::_AcquireGroupId(
    HdPrman_RenderParam* param,
    const _FlattenData& flattenGroup,
    riley::GeometryPrototypeId& groupId)
{
    HD_TRACE_FUNCTION();
    // Bail before locking if there's no parent instancer
    if (_Depth() == 0) {
        groupId = riley::GeometryPrototypeId::InvalidId();
        return false;
    }

    // This lock prevents concurrent calls to Populate from creating separate
    // riley groups for the same set of flatten data.
    tbb::spin_rw_mutex::scoped_lock lock(_groupIdAcquisitionLock, false);
    
    // We use the flatten data to look up whether this instancer has
    // a riley group that it will use for all instances across all
    // prototypes that will share that flatten data. When this instancer
    // tells its parent instancer to make instances of its riley prototype
    // groups, it will also tell the parent to set the params that make up
    // the flatten data on those instances (or pass them along to *its*
    // parent if it is not the outermost instancer). Using the hashable
    // FlattenData structure as the key to identify a group id allows us to
    // take maximal advantage of prman's support for nested instancing while
    // still preserving any incompatable params we may encounter.
    //
    // In short, If the instancer detects variability in the flatten data
    // across its instances, it will put them into separate buckets.

    groupId = _groupMap.get(flattenGroup);
    if (groupId == riley::GeometryPrototypeId::InvalidId()) {

        bool found = false;
        if (!lock.upgrade_to_writer()) {
            groupId = _groupMap.get(flattenGroup);
            if (groupId != riley::GeometryPrototypeId::InvalidId()) {
                found = true;
            }
        }

        if (!found) {
            RtPrimVarList groupPrimvars;
            groupPrimvars.SetString(RixStr.k_stats_prototypeIdentifier,
                RtUString(GetId().GetText()));
            groupId = param->AcquireRiley()->CreateGeometryPrototype(
                riley::UserId(stats::AddDataLocation(GetId().GetText()).GetValue()),
                RixStr.k_Ri_Group,
                riley::DisplacementId::InvalidId(),
                groupPrimvars);
            _groupMap.set(flattenGroup, groupId);
            return true;
        }
    }
    return false;
}

HdPrmanInstancer*
HdPrmanInstancer::_GetParentInstancer()
{
    // XXX: There is no way of knowing at this stage whether a native instancer
    // is part of a prototype of another instancer, and thus no way to access
    // the parent instancer for a native instancing-backed HdInstancer. This
    // will always return nullptr under native instancing, so native instancing
    // always produces full flattening in riley and takes no advantage of
    // nesting.
    //
    // Note that it is possible for instancers to have multiple parent
    // instancers! UsdImaging currently hides this behind instancer id munging
    // for point instancers, while native instancers do not propagate parent
    // data to hydra at all, so for now we assume only a single parent.

    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    SdfPath parentId = GetParentId();
    while (!parentId.IsEmpty() && !parentId.IsAbsoluteRootPath()) {
        auto* instancer = static_cast<HdPrmanInstancer*>(
            renderIndex.GetInstancer(parentId));
        if (instancer) {
            return instancer;
        }
        parentId = parentId.GetParentPath();
    }
    return nullptr;
}

int
HdPrmanInstancer::_Depth()
{
    // XXX: Since there is no way to tell if a native instancer has any parent
    // instancers, this will always return depth 0 for native instancing. Also
    // note that while it is technically possible for an instancer to have
    // multiple parent instancers, and multple depths along various paths to
    // its outermost ancestor instancers, assuming a single depth works for now
    // thanks to limitations imposed by UsdImaging.

    int depth = 0;
    HdPrmanInstancer* parent = _GetParentInstancer();
    while (parent) {
        depth++;
        parent = parent->_GetParentInstancer();
    }
    return depth;
}

void
HdPrmanInstancer::_GetInstanceParams(
    const size_t instanceIndex,
    RtParamList& params)
{
    for (const auto& entry : _primvarMap) {
        const HdPrimvarDescriptor& primvar = entry.second.desc;
        
        // 'constant' and 'uniform' USD primvars are inherited in toto by
        // instances, while 'varying', 'vertex', and 'faceVarying' USD primvars
        // (and those marked as instance-rate by hydra) are inherited per
        // instance by indexing into the value array.
        // See https://tinyurl.com/hdxya2yk.

        bool isConstantRate = primvar.interpolation == HdInterpolationConstant
            || primvar.interpolation == HdInterpolationUniform;

        // Confirm that instance-rate USD primvars are array-valued and have
        // sufficient dimensions.
        VtValue val = entry.second.value; // *not* a reference!
        if ((!isConstantRate) && (instanceIndex >= val.GetArraySize())) {
            TF_WARN("HdPrman: Instance-rate USD primvar has array size %zu; "
                    "cannot provide a value for instance index %zu\n",
                    val.GetArraySize(), instanceIndex);
            continue;
        }

        // If the interpolation is not constant or uniform and the value is
        // an array, extract just the value of interest.
        if ((!isConstantRate) && val.IsArrayValued()) {
            val = VtVisitValue(val, _GetValueAtIndex(instanceIndex));
        }

        const RtUString name = _FixupParamName(entry.first);
        
        // USD primvars in the "ri:attributes" and "primvars:ri:attributes"
        // namespaces end up having the same fixed-up param name, potentially
        // causing collisions in the param list. When both "ri:attributes" and
        // "primvar:ri:attributes" versions of the same USD primvar exist, the
        // "primvar:ri:attributes" version should win out.
        if (TfStringStartsWith(entry.first.GetString(), "ri:attributes:") &&
            params.HasParam(name)) {
            continue;
        }
        if (!HdPrman_Utils::SetParamFromVtValue(
                name, val, primvar.role, &params)) {
            TF_WARN("Unrecognized USD primvar value type at %s.%s",
                GetId().GetText(), entry.first.GetText());
        }
    }
}

void
HdPrmanInstancer::_GetPrototypeParams(
    const SdfPath& protoPath,
    RtParamList& params)
{
    // XXX: With the scene index enabled (Hydra 2.0), this fails to find
    // constant inherited USD primvars, but picks up displayColor and
    // displayOpacity, even when those are not authored anywhere on or
    // above the target prototype.

    HdSceneDelegate* delegate = GetDelegate();
    // Only get constant and uniform USD primvars
    for (HdInterpolation i = HdInterpolationConstant;
        i < HdInterpolationVarying; i = HdInterpolation(i+1)) {
        for (const HdPrimvarDescriptor& primvar :
            delegate->GetPrimvarDescriptors(protoPath, i)) {
            const RtUString name = _FixupParamName(primvar.name);

            // USD primvars in the "ri:attributes" and "primvars:ri:attributes"
            // namespaces end up having the same fixed-up param name, potentially
            // causing collisions in the param list. When both "ri:attributes" and
            // "primvar:ri:attributes" versions of the same USD primvar exist, the
            // "primvar:ri:attributes" version should win out.
            if (TfStringStartsWith(primvar.name.GetString(), "ri:attributes") &&
                params.HasParam(name)) {
                continue;
            }
            const VtValue& val = delegate->Get(protoPath, primvar.name);
            if (!HdPrman_Utils::SetParamFromVtValue(
                name, val, primvar.role, &params)) {
                TF_WARN("Unrecognized USD primvar value type at %s.%s",
                    protoPath.GetText(), primvar.name.GetText());
            }
        }
    }
}

void
HdPrmanInstancer::_GetInstanceTransform(
    const size_t instanceIndex,
    _GfMatrixSA& xform,
    const _GfMatrixSA& left)
{
    if (_sa.count > 0 && instanceIndex < _sa.values[0].size()) {
        if (left.count > 0) {
            _GfMatrixSA right;
            _AccumulateSampleTimes(_sa, right);
            for (size_t i = 0; i < right.count; ++i) {
                right.values[i] = _sa.values[i][instanceIndex];
            }
            _AccumulateSampleTimes(left, xform);
            _AccumulateSampleTimes(right, xform);
            for (size_t i = 0; i < xform.count; ++i) {
                const float t = xform.times[i];
                xform.values[i] = left.Resample(t) * right.Resample(t);
            }
        } else {
            _AccumulateSampleTimes(_sa, xform);
            for (size_t i = 0; i < xform.count; ++i) {
                xform.values[i] = _sa.values[i][instanceIndex];
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
