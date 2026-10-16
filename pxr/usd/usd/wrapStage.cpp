//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/ar/resolverContext.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/pcp/pyUtils.h"
#include "pxr/usd/sdf/pyUtils.h"

#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

class Usd_PcpCacheAccess
{
public:
    static const PcpCache* GetPcpCache(const UsdStage& stage)
    { return stage._GetPcpCache(); }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Export(const UsdStagePtr &self, const std::string& filename, 
        bool addSourceFileComment, const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    std::string errMsg;
    if (!SdfFileFormatArgumentsFromPython(dict, &args, &errMsg)) {
        TF_CODING_ERROR("%s", errMsg.c_str());
        return false;
    }

    return self->Export(filename, addSourceFileComment, args);
}

static string
_ExportToString(const UsdStagePtr &self, bool addSourceFileComment=true)
{
    string result;
    self->ExportToString(&result, addSourceFileComment);
    return result;
}

static string
__repr__(nb::handle self)
{
    if (!nb::inst_ready(self)) {
        return "invalid " + UsdDescribe(UsdStagePtr());
    }
    UsdStagePtr stage(nb::cast<UsdStage *>(self));
    if (stage.IsExpired()) {
        return "invalid " + UsdDescribe(stage);
    }
    
    string result = TF_PY_REPR_PREFIX + TfStringPrintf(
        "Stage.Open(rootLayer=%s, sessionLayer=%s",
        TfPyRepr(stage->GetRootLayer()).c_str(),
        TfPyRepr(stage->GetSessionLayer()).c_str());
        
    if (!stage->GetPathResolverContext().IsEmpty()) {
        result += TfStringPrintf(
            ", pathResolverContext=%s",
            TfPyRepr(stage->GetPathResolverContext()).c_str());
    }

    return result + ")";
}

static TfPyObjWrapper
_GetMetadata(const UsdStagePtr &self, const TfToken &key)
{
    VtValue  result;
    self->GetMetadata(key, &result);
    // If the above failed, result will still be empty, which is
    // the appropriate return value
    return UsdVtValueToPython(result);
}

static bool _SetMetadata(const UsdStagePtr &self, const TfToken& key,
                         nb::handle obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, /*keyPath*/TfToken(), obj, &value) &&
        self->SetMetadata(key, value);
}

static TfPyObjWrapper
_GetMetadataByDictKey(const UsdStagePtr &self, 
                      const TfToken &key, const TfToken &keyPath)
{
    VtValue  result;
    self->GetMetadataByDictKey(key, keyPath, &result);
    // If the above failed, result will still be empty, which is
    // the appropriate return value
    return UsdVtValueToPython(result);
}

static bool _SetMetadataByDictKey(const UsdStagePtr &self, const TfToken& key,
                                  const TfToken &keyPath, nb::handle obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, keyPath, obj, &value) &&
        self->SetMetadataByDictKey(key, keyPath, value);
}

static void
_SetGlobalVariantFallbacks(const nb::dict & d)
{
    PcpVariantFallbackMap fallbacks;
    if (PcpVariantFallbackMapFromPython(d, &fallbacks)) {
        UsdStage::SetGlobalVariantFallbacks(fallbacks);
    }
}

static UsdEditTarget
_GetEditTargetForLocalLayerIndex(const UsdStagePtr &self, size_t index)
{
    return self->GetEditTargetForLocalLayer(index);
}

static UsdEditTarget
_GetEditTargetForLocalLayer(const UsdStagePtr &self,
                            const SdfLayerHandle &layer)
{
    return self->GetEditTargetForLocalLayer(layer);
}

static void
_ExpandPopulationMask(UsdStage &self,
                      Usd_PrimFlagsPredicate const &traversal,
                      nb::object pyRelPred,
                      nb::object pyAttrPred)
{
    using RelPredicate = std::function<bool (UsdRelationship const &)>;
    using AttrPredicate = std::function<bool (UsdAttribute const &)>;
    RelPredicate relPred;
    AttrPredicate attrPred;
    if (!pyRelPred.is_none()) {
        relPred = nb::cast<RelPredicate>(pyRelPred);
    }
    if (!pyAttrPred.is_none()) {
        attrPred = nb::cast<AttrPredicate>(pyAttrPred);
    }
    return self.ExpandPopulationMask(traversal, relPred, attrPred);
}

static void
_ExpandPopulationMaskDefault(UsdStage &self,
                             nb::object pyRelPred,
                             nb::object pyAttrPred)
{
    Usd_PrimFlagsPredicate traversal;
    if (!pyRelPred.is_none() &&
        nb::try_cast<Usd_PrimFlagsPredicate>(pyRelPred, traversal)) {
        return _ExpandPopulationMask(
            self, traversal, pyAttrPred, nb::none());
    }
    return _ExpandPopulationMask(
        self, UsdPrimDefaultPredicate, pyRelPred, pyAttrPred);
}

static nb::tuple
_GetColorConfigFallbacks()
{
    SdfAssetPath colorConfiguration;
    TfToken colorManagementSystem;
    UsdStage::GetColorConfigFallbacks(&colorConfiguration, 
                                      &colorManagementSystem);
    return nb::make_tuple(colorConfiguration, colorManagementSystem);
}

} // anonymous namespace 

void wrapUsdStage(nb::module_ &m)
{
    nb::class_<UsdStage> cls(
        m, "Stage", nb::is_weak_referenceable(), nb::dynamic_attr());

    // Expose the UsdStage::InitialLoadSet enum under the Usd.Stage scope.
    // We need to do this here because we use enum values as default
    // parameters to other wrapped functions.
    TfNbWrapEnum<UsdStage::InitialLoadSet>(cls);

    cls
        .def(TfNbRefAndWeakPtr())
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("__repr__", __repr__)

        .def_static("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             nb::arg("identifier"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const ArResolverContext &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             nb::arg("identifier"), 
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const SdfLayerHandle &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             nb::arg("identifier"), 
             nb::arg("sessionLayer"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const SdfLayerHandle &,
                                              const ArResolverContext &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             nb::arg("identifier"), 
             nb::arg("sessionLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)

        .def_static("CreateInMemory", (UsdStageRefPtr (*)(
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             nb::arg("identifier"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const ArResolverContext &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             nb::arg("identifier"), 
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const SdfLayerHandle &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             nb::arg("identifier"), 
             nb::arg("sessionLayer"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const SdfLayerHandle &,
                                    const ArResolverContext &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             nb::arg("identifier"),
             nb::arg("sessionLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)

        .def_static("Open", (UsdStageRefPtr (*)(const string &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("filePath"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("Open", (UsdStageRefPtr (*)(const string &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("filePath"),
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)

        .def_static("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("rootLayer"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("rootLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("rootLayer"),
             nb::arg("sessionLayer"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             nb::arg("rootLayer"),
             nb::arg("sessionLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("load")=UsdStage::LoadAll)

        .def_static("OpenMasked", (UsdStageRefPtr (*)(const string &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("filePath"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("OpenMasked", (UsdStageRefPtr (*)(const string &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("filePath"),
             nb::arg("pathResolverContext"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("rootLayer"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("rootLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("rootLayer"),
             nb::arg("sessionLayer"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        .def_static("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             nb::arg("rootLayer"),
             nb::arg("sessionLayer"),
             nb::arg("pathResolverContext"),
             nb::arg("mask"),
             nb::arg("load")=UsdStage::LoadAll)
        
        .def("Reload", &UsdStage::Reload)

        .def("Save", &UsdStage::Save)
        .def("SaveSessionLayers", &UsdStage::SaveSessionLayers)
        .def("WriteFallbackPrimTypes", &UsdStage::WriteFallbackPrimTypes)

        .def_static("GetGlobalVariantFallbacks",
             &UsdStage::GetGlobalVariantFallbacks)
        .def_static("SetGlobalVariantFallbacks", &_SetGlobalVariantFallbacks)

        .def("Load", &UsdStage::Load,
             nb::arg("path")=SdfPath::AbsoluteRootPath(),
             nb::arg("policy")=UsdLoadWithDescendants)

        .def("Unload", &UsdStage::Unload,
             nb::arg("path")=SdfPath::AbsoluteRootPath())

        .def("LoadAndUnload", &UsdStage::LoadAndUnload,
             nb::arg("loadSet"), nb::arg("unloadSet"),
             nb::arg("policy")=UsdLoadWithDescendants)

        .def("GetLoadSet", [](UsdStage &self) {
            return TfNbSequenceToList(self.GetLoadSet());
        })

        .def("FindLoadable", [](UsdStage &self, const SdfPath &rootPath) {
            return TfNbSequenceToList(self.FindLoadable(rootPath));
        }, nb::arg("rootPath")=SdfPath::AbsoluteRootPath())

        .def("GetLoadRules", &UsdStage::GetLoadRules,
             nb::rv_policy::copy)
        .def("SetLoadRules", &UsdStage::SetLoadRules, nb::arg("rules"))

        .def("GetPopulationMask", &UsdStage::GetPopulationMask)
        .def("SetPopulationMask", &UsdStage::SetPopulationMask, nb::arg("mask"))
        .def("ExpandPopulationMask", &_ExpandPopulationMask,
             nb::arg("traversalPredicate"),
             nb::arg("relationshipPredicate").none()=nb::none(),
             nb::arg("attributePredicate").none()=nb::none())
        .def("ExpandPopulationMask", &_ExpandPopulationMaskDefault,
             nb::arg("relationshipPredicate").none()=nb::none(),
             nb::arg("attributePredicate").none()=nb::none())

        .def("GetPseudoRoot", &UsdStage::GetPseudoRoot)

        .def("GetDefaultPrim", &UsdStage::GetDefaultPrim)
        .def("SetDefaultPrim", &UsdStage::SetDefaultPrim, nb::arg("prim"))
        .def("ClearDefaultPrim", &UsdStage::ClearDefaultPrim)
        .def("HasDefaultPrim", &UsdStage::HasDefaultPrim)

        .def("GetPrimAtPath", &UsdStage::GetPrimAtPath, nb::arg("path"))
        .def("GetObjectAtPath", &UsdStage::GetObjectAtPath, nb::arg("path"))
        .def("GetPropertyAtPath", &UsdStage::GetPropertyAtPath, nb::arg("path"))
        .def("GetAttributeAtPath", &UsdStage::GetAttributeAtPath, nb::arg("path"))
        .def("GetRelationshipAtPath", &UsdStage::GetRelationshipAtPath, nb::arg("path"))
        .def("Traverse", (UsdPrimRange (UsdStage::*)())
             &UsdStage::Traverse)
        .def("Traverse",
             (UsdPrimRange (UsdStage::*)(const Usd_PrimFlagsPredicate &))
             &UsdStage::Traverse, nb::arg("predicate"))
        .def("TraverseAll", &UsdStage::TraverseAll)

        .def("OverridePrim", &UsdStage::OverridePrim, nb::arg("path"))
        .def("DefinePrim", &UsdStage::DefinePrim,
             nb::arg("path"), nb::arg("typeName")=TfToken())
        .def("CreateClassPrim", &UsdStage::CreateClassPrim, nb::arg("rootPrimPath"))

        .def("RemovePrim", &UsdStage::RemovePrim, nb::arg("path"))

        .def("GetSessionLayer", &UsdStage::GetSessionLayer)
        .def("GetRootLayer", &UsdStage::GetRootLayer)
        .def("GetPathResolverContext", &UsdStage::GetPathResolverContext)
        .def("ResolveIdentifierToEditTarget",
             &UsdStage::ResolveIdentifierToEditTarget, nb::arg("identifier"))
        .def("GetCompositionErrors", &UsdStage::GetCompositionErrors)
        .def("GetLayerStack", &UsdStage::GetLayerStack,
             nb::arg("includeSessionLayers")=true)
        .def("GetUsedLayers", &UsdStage::GetUsedLayers,
             nb::arg("includeClipLayers")=true)

        .def("HasLocalLayer", &UsdStage::HasLocalLayer, nb::arg("layer"))

        .def("GetEditTarget", &UsdStage::GetEditTarget,
             nb::rv_policy::copy)
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayerIndex,
             nb::rv_policy::copy)
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayer,
             nb::rv_policy::copy)
        .def("SetEditTarget", &UsdStage::SetEditTarget, nb::arg("editTarget"))

        .def("MuteLayer", &UsdStage::MuteLayer,
             nb::arg("layerIdentifier"))
        .def("UnmuteLayer", &UsdStage::UnmuteLayer,
             nb::arg("layerIdentifier"))
        .def("MuteAndUnmuteLayers", &UsdStage::MuteAndUnmuteLayers,
             nb::arg("muteLayers"),
             nb::arg("unmuteLayers"))
        .def("GetMutedLayers", &UsdStage::GetMutedLayers,
             nb::rv_policy::copy)
        .def("IsLayerMuted", &UsdStage::IsLayerMuted,
             nb::arg("layerIdentifier"))

        .def("Export", &_Export,
             nb::arg("filename"), 
             nb::arg("addSourceFileComment")=true,
             nb::arg("args") = nb::dict())

        .def("ExportToString", _ExportToString,
             nb::arg("addSourceFileComment")=true)

        .def("Flatten", &UsdStage::Flatten,
             nb::arg("addSourceFileComment")=true)

        .def("GetMetadata", &_GetMetadata)
        .def("HasMetadata", &UsdStage::HasMetadata)
        .def("HasAuthoredMetadata", &UsdStage::HasAuthoredMetadata)
        .def("ClearMetadata", &UsdStage::ClearMetadata)
        .def("SetMetadata", &_SetMetadata)

        .def("GetMetadataByDictKey", &_GetMetadataByDictKey)
        .def("HasMetadataDictKey", &UsdStage::HasMetadataDictKey)
        .def("HasAuthoredMetadataDictKey", &UsdStage::HasAuthoredMetadataDictKey)
        .def("ClearMetadataByDictKey", &UsdStage::ClearMetadataByDictKey)
        .def("SetMetadataByDictKey", &_SetMetadataByDictKey)

        .def("GetStartTimeCode", &UsdStage::GetStartTimeCode)
        .def("SetStartTimeCode", &UsdStage::SetStartTimeCode)

        .def("GetEndTimeCode", &UsdStage::GetEndTimeCode)
        .def("SetEndTimeCode", &UsdStage::SetEndTimeCode)

        .def("HasAuthoredTimeCodeRange", &UsdStage::HasAuthoredTimeCodeRange)

        .def("GetTimeCodesPerSecond", &UsdStage::GetTimeCodesPerSecond)
        .def("SetTimeCodesPerSecond", &UsdStage::SetTimeCodesPerSecond)

        .def("GetFramesPerSecond", &UsdStage::GetFramesPerSecond)
        .def("SetFramesPerSecond", &UsdStage::SetFramesPerSecond)

        .def("GetColorConfiguration", &UsdStage::GetColorConfiguration)
        .def("SetColorConfiguration", &UsdStage::SetColorConfiguration)

        .def("GetColorManagementSystem", &UsdStage::GetColorManagementSystem)
        .def("SetColorManagementSystem", &UsdStage::SetColorManagementSystem)

        .def_static("GetColorConfigFallbacks", &_GetColorConfigFallbacks)

        .def_static("SetColorConfigFallbacks", &UsdStage::SetColorConfigFallbacks,
             nb::arg("colorConfiguration")=SdfAssetPath(), 
             nb::arg("colorManagementSystem")=TfToken())
            
        .def("GetInterpolationType", &UsdStage::GetInterpolationType)
        .def("SetInterpolationType", &UsdStage::SetInterpolationType)
        .def_static("IsSupportedFile", &UsdStage::IsSupportedFile, nb::arg("filePath"))

        .def("GetPrototypes", &UsdStage::GetPrototypes)

        .def("_GetPcpCache", &Usd_PcpCacheAccess::GetPcpCache,
             nb::rv_policy::reference_internal)
        ;
}
