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
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/conversions.h"
#include "pxr/usd/usd/treeIterator.h"

#include "pxr/usd/ar/resolverContext.h"
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

PXR_NAMESPACE_OPEN_SCOPE


using std::string;

using namespace boost::python;

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

static string
_ExportToString(const UsdStagePtr &self, bool addSourceFileComment=true)
{
    string result;
    self->ExportToString(&result, addSourceFileComment);
    return result;
}

static string
__repr__(const UsdStagePtr &self)
{
    string result = TF_PY_REPR_PREFIX + TfStringPrintf(
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
                         object obj) {
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
                                  const TfToken &keyPath, object obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, keyPath, obj, &value) &&
        self->SetMetadataByDictKey(key, keyPath, value);
}

static void
_SetGlobalVariantFallbacks(const dict& d)
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
_ExpandPopulationMask(UsdStage &self, boost::python::object pypred)
{
    using Predicate = std::function<bool (UsdRelationship const &)>;
    Predicate pred;
    if (pypred != boost::python::object())
        pred = boost::python::extract<Predicate>(pypred);
    return self.ExpandPopulationMask(pred);
}

void wrapUsdStage()
{
    typedef TfWeakPtr<UsdStage> StagePtr;

    class_<UsdStage, StagePtr, boost::noncopyable>
        cls("Stage", no_init)
        ;

    // Expose the UsdStage::InitialLoadSet enum under the Usd.Stage scope.
    // We need to do this here because we use enum values as default
    // parameters to other wrapped functions.
    scope s = cls;
    TfPyWrapEnum<UsdStage::InitialLoadSet>();

    cls
        .def(TfPyRefAndWeakPtr())
        .def("__repr__", __repr__)

        .def("CreateNew", (UsdStageRefPtr (*)(const string &))
             &UsdStage::CreateNew,
             (arg("identifier")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const SdfLayerHandle &))
             &UsdStage::CreateNew,
             (arg("identifier"), arg("sessionLayer")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const ArResolverContext &))
             &UsdStage::CreateNew,
             (arg("identifier"), arg("pathResolverContext")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateNew", (UsdStageRefPtr (*)(const string &,
                                              const SdfLayerHandle &,
                                              const ArResolverContext &))
             &UsdStage::CreateNew,
             (arg("identifier"), arg("sessionLayer"),
              arg("pathResolverContext")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("CreateNew")

        .def("CreateInMemory", (UsdStageRefPtr (*)())
             &UsdStage::CreateInMemory,
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(const string &))
             &UsdStage::CreateInMemory,
             (arg("identifier")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const ArResolverContext &))
             &UsdStage::CreateInMemory,
             (arg("identifier"), arg("pathResolverContext")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const SdfLayerHandle &))
             &UsdStage::CreateInMemory,
             (arg("identifier"), arg("sessionLayer")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("CreateInMemory", (UsdStageRefPtr (*)(
                                    const string &,
                                    const SdfLayerHandle &,
                                    const ArResolverContext &))
             &UsdStage::CreateInMemory,
             (arg("identifier"),
              arg("sessionLayer"),
              arg("pathResolverContext")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("CreateInMemory")

        .def("Open", (UsdStageRefPtr (*)(const string &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("filePath"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const string &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("filePath"),
              arg("pathResolverContext"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())

        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("rootLayer"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("rootLayer"),
              arg("sessionLayer"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("rootLayer"),
              arg("pathResolverContext"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("Open", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                         const SdfLayerHandle &,
                                         const ArResolverContext &, 
                                         UsdStage::InitialLoadSet))
             &UsdStage::Open,
             (arg("rootLayer"),
              arg("sessionLayer"),
              arg("pathResolverContext"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("Open")

        .def("OpenMasked", (UsdStageRefPtr (*)(const string &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("filePath"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const string &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("filePath"),
              arg("pathResolverContext"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("rootLayer"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("rootLayer"),
              arg("sessionLayer"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("rootLayer"),
              arg("pathResolverContext"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("OpenMasked", (UsdStageRefPtr (*)(const SdfLayerHandle &,
                                               const SdfLayerHandle &,
                                               const ArResolverContext &, 
                                               const UsdStagePopulationMask &,
                                               UsdStage::InitialLoadSet))
             &UsdStage::OpenMasked,
             (arg("rootLayer"),
              arg("sessionLayer"),
              arg("pathResolverContext"),
              arg("mask"),
              arg("load")=UsdStage::LoadAll),
             return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("OpenMasked")
        
        .def("Close", &UsdStage::Close)
        .def("Reload", &UsdStage::Reload)

        .def("GetGlobalVariantFallbacks",
             &UsdStage::GetGlobalVariantFallbacks,
             return_value_policy<TfPyMapToDictionary>())
        .staticmethod("GetGlobalVariantFallbacks")
        .def("SetGlobalVariantFallbacks", &_SetGlobalVariantFallbacks)
        .staticmethod("SetGlobalVariantFallbacks")

        .def("Load", &UsdStage::Load,
             arg("path")=SdfPath::AbsoluteRootPath())

        .def("Unload", &UsdStage::Unload,
             arg("path")=SdfPath::AbsoluteRootPath())

        .def("LoadAndUnload", &UsdStage::LoadAndUnload,
             (arg("loadSet"), arg("unloadSet")))

        .def("GetLoadSet", &UsdStage::GetLoadSet,
             return_value_policy<TfPySequenceToList>())

        .def("FindLoadable", &UsdStage::FindLoadable,
             arg("rootPath")=SdfPath::AbsoluteRootPath(),
             return_value_policy<TfPySequenceToList>())

        .def("GetPopulationMask", &UsdStage::GetPopulationMask)
        .def("SetPopulationMask", &UsdStage::SetPopulationMask, arg("mask"))
        .def("ExpandPopulationMask", &_ExpandPopulationMask,
             arg("predicate")=object())

        .def("GetPseudoRoot", &UsdStage::GetPseudoRoot)

        .def("GetDefaultPrim", &UsdStage::GetDefaultPrim)
        .def("SetDefaultPrim", &UsdStage::SetDefaultPrim, arg("prim"))
        .def("ClearDefaultPrim", &UsdStage::ClearDefaultPrim)
        .def("HasDefaultPrim", &UsdStage::HasDefaultPrim)

        .def("GetPrimAtPath", &UsdStage::GetPrimAtPath, arg("path"))
        .def("Traverse", (UsdTreeIterator (UsdStage::*)())
             &UsdStage::Traverse)
        .def("Traverse",
             (UsdTreeIterator (UsdStage::*)(const Usd_PrimFlagsPredicate &))
             &UsdStage::Traverse, arg("predicate"))
        .def("TraverseAll", &UsdStage::TraverseAll)

        .def("OverridePrim", &UsdStage::OverridePrim, arg("path"))
        .def("DefinePrim", &UsdStage::DefinePrim,
             (arg("path"), arg("typeName")=TfToken()))
        .def("CreateClassPrim", &UsdStage::CreateClassPrim, arg("rootPrimPath"))

        .def("RemovePrim", &UsdStage::RemovePrim, arg("path"))

        .def("GetSessionLayer", &UsdStage::GetSessionLayer)
        .def("GetRootLayer", &UsdStage::GetRootLayer)
        .def("GetPathResolverContext", &UsdStage::GetPathResolverContext)
        .def("ResolveIdentifierToEditTarget",
             &UsdStage::ResolveIdentifierToEditTarget, arg("identifier"))
        .def("GetLayerStack", &UsdStage::GetLayerStack,
             arg("includeSessionLayers")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetUsedLayers", &UsdStage::GetUsedLayers,
             arg("includeClipLayers")=true,
             return_value_policy<TfPySequenceToList>())

        .def("HasLocalLayer", &UsdStage::HasLocalLayer, arg("layer"))

        .def("GetEditTarget", &UsdStage::GetEditTarget,
             return_value_policy<return_by_value>())
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayerIndex,
             return_value_policy<return_by_value>())
        .def("GetEditTargetForLocalLayer", &_GetEditTargetForLocalLayer,
             return_value_policy<return_by_value>())
        .def("SetEditTarget", &UsdStage::SetEditTarget, arg("editTarget"))

        .def("MuteLayer", &UsdStage::MuteLayer,
             (arg("layerIdentifier")))
        .def("UnmuteLayer", &UsdStage::UnmuteLayer,
             (arg("layerIdentifier")))
        .def("MuteAndUnmuteLayers", &UsdStage::MuteAndUnmuteLayers,
             (arg("muteLayers"),
              arg("unmuteLayers")))
        .def("GetMutedLayers", &UsdStage::GetMutedLayers,
             return_value_policy<TfPySequenceToList>())
        .def("IsLayerMuted", &UsdStage::IsLayerMuted,
             (arg("layerIdentifier")))

        .def("Export", &_Export,
             (arg("filename"), 
              arg("addSourceFileComment")=true,
              arg("args") = boost::python::dict()))

        .def("ExportToString", _ExportToString,
             arg("addSourceFileComment")=true)

        .def("Flatten", &UsdStage::Flatten,
             (arg("addSourceFileComment")=true),
             return_value_policy<TfPyRefPtrFactory<SdfLayerHandle> >())

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

        .def("GetInterpolationType", &UsdStage::GetInterpolationType)
        .def("SetInterpolationType", &UsdStage::SetInterpolationType)
        .def("IsSupportedFile", &UsdStage::IsSupportedFile, arg("filePath"))
        .staticmethod("IsSupportedFile")

        .def("GetMasters", &UsdStage::GetMasters,
             return_value_policy<TfPySequenceToList>())
        ;
}

TF_REFPTR_CONST_VOLATILE_GET(UsdStage)

PXR_NAMESPACE_CLOSE_SCOPE
