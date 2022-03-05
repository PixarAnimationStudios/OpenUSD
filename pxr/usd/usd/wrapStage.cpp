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
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>



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
        bool addSourceFileComment, const boost::python::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    std::string errMsg;
    if (!SdfFileFormatArgumentsFromPython(dict, &args, &errMsg)) {
        TF_CODING_ERROR("%s", errMsg.c_str());
        return false;
    }

    return self->Export(filename, addSourceFileComment, args);
}

static std::string
_ExportToString(const UsdStagePtr &self, bool addSourceFileComment=true)
{
    std::string result;
    self->ExportToString(&result, addSourceFileComment);
    return result;
}

static std::string
__repr__(const UsdStagePtr &self)
{
    if (self.IsExpired()) {
        return "invalid " + UsdDescribe(self);
    }
    
    std::string result = TF_PY_REPR_PREFIX + TfStringPrintf(
        "Stage.Open(rootLayer=%s, sessionLayer=%s",
        TfPyRepr(self->GetRootLayer()).c_str(),
        TfPyRepr(self->GetSessionLayer()).c_str());
        
    if (!self->GetPathResolverContext().IsEmpty()) {
        result += TfStringPrintf(
            ", pathResolverContext=%s",
            TfPyRepr(self->GetPathResolverContext()).c_str());
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
                         boost::python::object obj) {
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
                                  const TfToken &keyPath, boost::python::object obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, keyPath, obj, &value) &&
        self->SetMetadataByDictKey(key, keyPath, value);
}

static void
_SetGlobalVariantFallbacks(const boost::python::dict& d)
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
                      boost::python::object pyRelPred,
                      boost::python::object pyAttrPred)
{
    using RelPredicate = std::function<bool (UsdRelationship const &)>;
    using AttrPredicate = std::function<bool (UsdAttribute const &)>;
    RelPredicate relPred;
    AttrPredicate attrPred;
    if (!pyRelPred.is_none()) {
        relPred = boost::python::extract<RelPredicate>(pyRelPred);
    }
    if (!pyAttrPred.is_none()) {
        attrPred = boost::python::extract<AttrPredicate>(pyAttrPred);
    }
    return self.ExpandPopulationMask(relPred, attrPred);
}

static boost::python::object 
_GetColorConfigFallbacks()
{
    SdfAssetPath colorConfiguration;
    TfToken colorManagementSystem;
    UsdStage::GetColorConfigFallbacks(&colorConfiguration, 
                                      &colorManagementSystem);
    return boost::python::make_tuple(colorConfiguration, colorManagementSystem);
}

} // anonymous namespace 

void wrapUsdStage()
{
    typedef TfWeakPtr<UsdStage> StagePtr;

    boost::python::class_<UsdStage, StagePtr, boost::noncopyable>
        cls("Stage", boost::python::no_init)
        ;

    // Expose the UsdStage::InitialLoadSet enum under the Usd.Stage scope.
    // We need to do this here because we use enum values as default
    // parameters to other wrapped functions.
    boost::python::scope s = cls;
    TfPyWrapEnum<UsdStage::InitialLoadSet>();

    cls
        .def(TfPyRefAndWeakPtr())
        .def("__repr__", __repr__)

        .def("CreateNew", (UsdStageRefPtr (*)(const std::string &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             (boost::python::arg("identifier"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const std::string &,
                                              const SdfLayerHandle &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             (boost::python::arg("identifier"), 
              boost::python::arg("sessionLayer"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const std::string &,
                                              const ArResolverContext &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             (boost::python::arg("identifier"), 
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const std::string &,
                                              const SdfLayerHandle &,
                                              const ArResolverContext &,
                                              UsdStage::InitialLoadSet))
             &UsdStage::CreateNew,
             (boost::python::arg("identifier"), 
              boost::python::arg("sessionLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("CreateNew")

        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             (boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const std::string &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             (boost::python::arg("identifier"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const std::string &,
                                    const ArResolverContext &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             (boost::python::arg("identifier"), 
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const std::string &,
                                    const SdfLayerHandle &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             (boost::python::arg("identifier"), 
              boost::python::arg("sessionLayer"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const std::string &,
                                    const SdfLayerHandle &,
                                    const ArResolverContext &,
                                    UsdStage::InitialLoadSet))
             &UsdStage::CreateInMemory,
             (boost::python::arg("identifier"),
              boost::python::arg("sessionLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("CreateInMemory")

        .def("Open", (UsdStageRefPtr (*)(const std::string &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("filePath"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const std::string &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("filePath"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())

        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("rootLayer"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("rootLayer"),
              boost::python::arg("sessionLayer"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("rootLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (boost::python::arg("rootLayer"),
              boost::python::arg("sessionLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("Open")

        .def("OpenMasked", (UsdStageRefPtr (*)(const std::string &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("filePath"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const std::string &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("filePath"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("rootLayer"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("rootLayer"),
              boost::python::arg("sessionLayer"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("rootLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (boost::python::arg("rootLayer"),
              boost::python::arg("sessionLayer"),
              boost::python::arg("pathResolverContext"),
              boost::python::arg("mask"),
              boost::python::arg("load")=UsdStage::LoadAll),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("OpenMasked")
        
        .def("Reload", &UsdStage::Reload)

        .def("Save", &UsdStage::Save)
        .def("SaveSessionLayers", &UsdStage::SaveSessionLayers)
        .def("WriteFallbackPrimTypes", &UsdStage::WriteFallbackPrimTypes)

        .def("GetGlobalVariantFallbacks",
             &UsdStage::GetGlobalVariantFallbacks,
             boost::python::return_value_policy<TfPyMapToDictionary>())
        .staticmethod("GetGlobalVariantFallbacks")
        .def("SetGlobalVariantFallbacks", &_SetGlobalVariantFallbacks)
        .staticmethod("SetGlobalVariantFallbacks")

        .def("Load", &UsdStage::Load,
             (boost::python::arg("path")=SdfPath::AbsoluteRootPath(),
              boost::python::arg("policy")=UsdLoadWithDescendants))

        .def("Unload", &UsdStage::Unload,
             boost::python::arg("path")=SdfPath::AbsoluteRootPath())

        .def("LoadAndUnload", &UsdStage::LoadAndUnload,
             (boost::python::arg("loadSet"), boost::python::arg("unloadSet"),
              boost::python::arg("policy")=UsdLoadWithDescendants))

        .def("GetLoadSet", &UsdStage::GetLoadSet,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("FindLoadable", &UsdStage::FindLoadable,
             boost::python::arg("rootPath")=SdfPath::AbsoluteRootPath(),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetLoadRules", &UsdStage::GetLoadRules,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetLoadRules", &UsdStage::SetLoadRules, boost::python::arg("rules"))

        .def("GetPopulationMask", &UsdStage::GetPopulationMask)
        .def("SetPopulationMask", &UsdStage::SetPopulationMask, boost::python::arg("mask"))
        .def("ExpandPopulationMask", &_ExpandPopulationMask,
             (boost::python::arg("relationshipPredicate")=boost::python::object(),
              boost::python::arg("attributePredicate")=boost::python::object()))

        .def("GetPseudoRoot", &UsdStage::GetPseudoRoot)

        .def("GetDefaultPrim", &UsdStage::GetDefaultPrim)
        .def("SetDefaultPrim", &UsdStage::SetDefaultPrim, boost::python::arg("prim"))
        .def("ClearDefaultPrim", &UsdStage::ClearDefaultPrim)
        .def("HasDefaultPrim", &UsdStage::HasDefaultPrim)

        .def("GetPrimAtPath", &UsdStage::GetPrimAtPath, boost::python::arg("path"))
        .def("GetObjectAtPath", &UsdStage::GetObjectAtPath, boost::python::arg("path"))
        .def("GetPropertyAtPath", &UsdStage::GetPropertyAtPath, boost::python::arg("path"))
        .def("GetAttributeAtPath", &UsdStage::GetAttributeAtPath, boost::python::arg("path"))
        .def("GetRelationshipAtPath", &UsdStage::GetRelationshipAtPath, boost::python::arg("path"))
        .def("Traverse", (UsdPrimRange (UsdStage::*)())
             &UsdStage::Traverse)
        .def("Traverse",
             (UsdPrimRange (UsdStage::*)(const Usd_PrimFlagsPredicate &))
             &UsdStage::Traverse, boost::python::arg("predicate"))
        .def("TraverseAll", &UsdStage::TraverseAll)

        .def("OverridePrim", &UsdStage::OverridePrim, boost::python::arg("path"))
        .def("DefinePrim", &UsdStage::DefinePrim,
             (boost::python::arg("path"), boost::python::arg("typeName")=TfToken()))
        .def("CreateClassPrim", &UsdStage::CreateClassPrim, boost::python::arg("rootPrimPath"))

        .def("RemovePrim", &UsdStage::RemovePrim, boost::python::arg("path"))

        .def("GetSessionLayer", &UsdStage::GetSessionLayer)
        .def("GetRootLayer", &UsdStage::GetRootLayer)
        .def("GetPathResolverContext", &UsdStage::GetPathResolverContext)
        .def("ResolveIdentifierToEditTarget",
             &UsdStage::ResolveIdentifierToEditTarget, boost::python::arg("identifier"))
        .def("GetLayerStack", &UsdStage::GetLayerStack,
             boost::python::arg("includeSessionLayers")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetUsedLayers", &UsdStage::GetUsedLayers,
             boost::python::arg("includeClipLayers")=true,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("HasLocalLayer", &UsdStage::HasLocalLayer, boost::python::arg("layer"))

        .def("GetEditTarget", &UsdStage::GetEditTarget,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayerIndex,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayer,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("SetEditTarget", &UsdStage::SetEditTarget, boost::python::arg("editTarget"))

        .def("MuteLayer", &UsdStage::MuteLayer,
             (boost::python::arg("layerIdentifier")))
        .def("UnmuteLayer", &UsdStage::UnmuteLayer,
             (boost::python::arg("layerIdentifier")))
        .def("MuteAndUnmuteLayers", &UsdStage::MuteAndUnmuteLayers,
             (boost::python::arg("muteLayers"),
              boost::python::arg("unmuteLayers")))
        .def("GetMutedLayers", &UsdStage::GetMutedLayers,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("IsLayerMuted", &UsdStage::IsLayerMuted,
             (boost::python::arg("layerIdentifier")))

        .def("Export", &_Export,
             (boost::python::arg("filename"), 
              boost::python::arg("addSourceFileComment")=true,
              boost::python::arg("args") = boost::python::dict()))

        .def("ExportToString", _ExportToString,
             boost::python::arg("addSourceFileComment")=true)

        .def("Flatten", &UsdStage::Flatten,
             (boost::python::arg("addSourceFileComment")=true),
             boost::python::return_value_policy<TfPyRefPtrFactory<SdfLayerHandle> >())

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

        .def("GetColorConfigFallbacks", &_GetColorConfigFallbacks)
        .staticmethod("GetColorConfigFallbacks")

        .def("SetColorConfigFallbacks", &UsdStage::SetColorConfigFallbacks,
            (boost::python::arg("colorConfiguration")=SdfAssetPath(), 
             boost::python::arg("colorManagementSystem")=TfToken()))
        .staticmethod("SetColorConfigFallbacks")        
            
        .def("GetInterpolationType", &UsdStage::GetInterpolationType)
        .def("SetInterpolationType", &UsdStage::SetInterpolationType)
        .def("IsSupportedFile", &UsdStage::IsSupportedFile, boost::python::arg("filePath"))
        .staticmethod("IsSupportedFile")

        .def("GetPrototypes", &UsdStage::GetPrototypes,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("_GetPcpCache", &Usd_PcpCacheAccess::GetPcpCache,
             boost::python::return_internal_reference<>())
        ;
}

TF_REFPTR_CONST_VOLATILE_GET(UsdStage)
