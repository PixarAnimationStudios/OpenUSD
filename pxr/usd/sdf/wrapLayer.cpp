//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapLayer.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pyChildrenProxy.h"
#include "pxr/usd/sdf/pyUtils.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

typedef SdfPyChildrenProxy<SdfLayer::RootPrimsView> RootPrimsProxy;

static
RootPrimsProxy
_WrapGetRootPrims(const SdfLayerHandle& layer)
{
    return RootPrimsProxy(layer->GetRootPrims(), "prim");
}

static bool
_WrapIsMuted(const SdfLayerHandle &layer)
{
    return layer->IsMuted();
}

class Sdf_SubLayerOffsetsProxy {
public:
    typedef Sdf_SubLayerOffsetsProxy This;

    Sdf_SubLayerOffsetsProxy(const SdfLayerHandle &layer) : _layer(layer)
    {
    }

    static void Wrap(nb::module_& m)
    {
        nb::class_<Sdf_SubLayerOffsetsProxy>(m, "SubLayerOffsetsProxy")
            .def("__len__", &This::_GetSize)
            .def("__eq__", &This::_EqVec)
            .def("__ne__", &This::_NeVec)
            .def("__eq__", &This::_EqProxy)
            .def("__ne__", &This::_NeProxy)
            .def("__getitem__", &This::_GetItemByIndex)
            .def("__getitem__", &This::_GetItemByPath)
            .def("__repr__", &This::_GetRepr)
            .def("count", &This::_Count)
            .def("copy", &This::_GetValues)
            .def("index", &This::_FindIndexForValue)
            .def("__setitem__", &This::_SetItemByIndex)
            .def("__setitem__", &This::_SetItemByPath)
            ;
    }

private:
    // Helper function to check for container expiry and return a raw ptr
    // to the container.  If the container has expired, we throw an
    // exception, which Boost catches and converts to a Python exception.
    SdfLayer *GetLayer() const
    {
        if (!_layer) {
            // CODE_COVERAGE_OFF
            // I cannot figure out a way to get a pointer to an expired
            // layer in python, so I have not been able to cover this...
            TfPyThrowRuntimeError("Expired layer");
            // CODE_COVERAGE_ON
        }
        
        return _layer.operator->();
    }

    size_t _GetSize() const
    {
        return GetLayer()->GetNumSubLayerPaths();
    }

    bool _EqVec(const SdfLayerOffsetVector& rhs)
    {
        return (_GetValues() == rhs);
    }

    bool _NeVec(const SdfLayerOffsetVector& rhs)
    {
        return !_EqVec(rhs);
    }

    bool _EqProxy(const This& rhs)
    {
        SdfLayerOffsetVector values    =     _GetValues();
        SdfLayerOffsetVector rhsValues = rhs._GetValues();
        return (values == rhsValues);
    }

    bool _NeProxy(const This& rhs)
    {
        return !_EqProxy(rhs);
    }

    SdfLayerOffsetVector _GetValues() const
    {
        return GetLayer()->GetSubLayerOffsets();
    }

    int _FindIndexForValue(const SdfLayerOffset& val) const
    {
        SdfLayerOffsetVector values = _GetValues();

        for (size_t i = 0; i < values.size(); ++i) {
            if (values[i] == val) {
                return i;
            }
        }

        return -1;
    }

    int _FindIndexForPath(const std::string& path) const
    {
        SdfSubLayerProxy paths = GetLayer()->GetSubLayerPaths();
        for (size_t i = 0, n = paths.size(); i < n; ++i) {
            if (paths[i] == path) {
                return i;
            }
        }
        return -1;
    }
    
    SdfLayerOffset _GetItemByIndex(int index) const
    {
        size_t size = GetLayer()->GetNumSubLayerPaths();
        index = TfPyNormalizeIndex(index, size, true);
        return GetLayer()->GetSubLayerOffset(index);
    }

    SdfLayerOffset _GetItemByPath(const std::string& path) const
    {
        int index = _FindIndexForPath(path);
        if (index == -1) {
            TfPyThrowIndexError
                (TfStringPrintf("path @%s@ not present in subLayerPaths",
                                path.c_str()));
        }
        
        return _GetItemByIndex(index);
    }

    void _SetItemByIndex(int index, const SdfLayerOffset& value)
    {
        int size = GetLayer()->GetNumSubLayerPaths();
        if (index == -1) {
            index = size;
        }
        if (index < 0 || index > size) {
            TfPyThrowIndexError("Index out of range");
        }
        GetLayer()->SetSubLayerOffset(value, index);
    }

    void _SetItemByPath(const std::string& path, const SdfLayerOffset& value)
    {
        int index = _FindIndexForPath(path);
        if (index == -1) {
            TfPyThrowIndexError
                (TfStringPrintf("path @%s@ not present in subLayerPaths",
                                path.c_str()));
        }

        _SetItemByIndex(index, value);
    }

    int _Count(const SdfLayerOffset& val)
    {
        SdfLayerOffsetVector values = _GetValues();
        return std::count(values.begin(), values.end(), val);
    }

    std::string _GetRepr() const
    {
        SdfLayerOffsetVector values = _GetValues();

        std::string result;
        TF_FOR_ALL(it, values) {
            if (!result.empty()) {
                result += ", ";
            }
            result += TfPyRepr(*it);
        }
        return "[" + result + "]";
    }

private:
    SdfLayerHandle _layer;
};

static Sdf_SubLayerOffsetsProxy 
_WrapGetSubLayerOffsets(const SdfLayerHandle &self)
{
    return Sdf_SubLayerOffsetsProxy(self);
}

////////////////////////////////////////////////////////////////////////

static bool
_ExtractFileFormatArguments(
    const nb::dict& dict,
    SdfLayer::FileFormatArguments* args)
{
    std::string errMsg;
    if (!SdfFileFormatArgumentsFromPython(dict, args, &errMsg)) {
        TF_CODING_ERROR("%s", errMsg.c_str());
        return false;
    }
    return true;
}

static std::string
_Repr(nb::handle self)
{
    if (!nb::inst_ready(self)) {
        return "<expired " + TF_PY_REPR_PREFIX + "Layer instance>";
    }
    SdfLayer* layer = nb::inst_ptr<SdfLayer>(self);
    if (!layer) {
        return "<expired " + TF_PY_REPR_PREFIX + "Layer instance>";
    }
    return TF_PY_REPR_PREFIX + "Find(" + TfPyRepr(layer->GetIdentifier()) + ")";
}

static std::string
_ExportToString(const SdfLayerHandle &layer)
{
    std::string result;
    layer->ExportToString(&result);
    return result;
}

static bool
_Export(
    const SdfLayerHandle& layer,
    const std::string& filename, 
    const std::string& comment,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return false;
    }

    return layer->Export(filename, comment, args);
}

static std::vector<TfToken>
_ApplyRootPrimOrder(
    const SdfLayerHandle& layer,
    const std::vector<TfToken>& primNames)
{
    std::vector<TfToken> result = primNames;
    layer->ApplyRootPrimOrder(&result);
    return result;
}

static nb::list
_ListTimeSamplesForPath(const SdfLayerHandle& layer, const SdfPath& path)
{
    return TfNbSequenceToList(layer->ListTimeSamplesForPath(path));
}

static size_t
_GetNumTimeSamplesForPath(const SdfLayerHandle& layer, const SdfPath& path)
{
    return layer->GetNumTimeSamplesForPath(path);
}

static VtValue
_QueryTimeSample( const SdfLayerHandle & layer,
                  const SdfPath & path, double time )
{
    VtValue value;
    layer->QueryTimeSample(path, time, &value);
    return value;
}

static nb::tuple
_GetBracketingTimeSamples(const SdfLayerHandle & layer, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = layer->GetBracketingTimeSamples(time, &tLower, &tUpper);
    return nb::make_tuple(found, tLower, tUpper);
}

static nb::tuple
_GetBracketingTimeSamplesForPath(const SdfLayerHandle & layer,
                                 const SdfPath & path, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = layer->GetBracketingTimeSamplesForPath(path, time,
                                                        &tLower, &tUpper);
    return nb::make_tuple(found, tLower, tUpper);
}

static nb::tuple
_GetPreviousTimeSampleForPath(const SdfLayerHandle &layer,
                              const SdfPath &path, double time)
{
    double tPrevious = 0;
    bool found = layer->GetPreviousTimeSampleForPath(path, time, &tPrevious);
    return nb::make_tuple(found, tPrevious);
}

static void
_SetTimeSample(const SdfLayerHandle& layer, const SdfPath& path,
               double time, const VtValue& value)
{
    layer->SetTimeSample(path, time, value);
}

static void
_EraseTimeSample(const SdfLayerHandle& layer, const SdfPath& path,
                 double time)
{
    layer->EraseTimeSample(path, time);
}

static nb::tuple
_SplitIdentifier(const std::string& identifier)
{
    std::string layerPath;
    SdfLayer::FileFormatArguments args;
    SdfLayer::SplitIdentifier(identifier, &layerPath, &args);
    return nb::make_tuple(layerPath, args);
}

static
nb::object
_CanApplyNamespaceEdit(
    const SdfLayerHandle& x,
    const SdfBatchNamespaceEdit& edit)
{
    SdfNamespaceEditDetailVector details;
    SdfNamespaceEditDetail::Result result = x->CanApply(edit, &details);
    if (result != SdfNamespaceEditDetail::Okay) {
        return nb::make_tuple(false, details);
    }
    else {
        return nb::bool_(true);
    }
}

static SdfLayerRefPtr
_CreateNew(
    const std::string& identifier,
    const nb::dict& dict = nb::dict())
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::CreateNew(identifier, args);
}

static SdfLayerRefPtr
_New(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier,
    const nb::dict& dict = nb::dict())
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::New(fileFormat, identifier, args);
}

static SdfLayerRefPtr
_CreateAnonymous(
    const std::string& tag,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::CreateAnonymous(tag, args);
}

static SdfLayerRefPtr
_CreateAnonymous(
    const std::string& tag,
    const SdfFileFormatConstPtr& fmt,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::CreateAnonymous(tag, fmt, args);
}

static SdfLayerRefPtr
_FindOrOpen(
    const std::string& identifier,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::FindOrOpen(identifier, args);
}

static SdfLayerHandle
_Find(
    const std::string& identifier,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    return SdfLayer::Find(identifier, args);
}

static SdfLayerHandle
_FindRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& identifier,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    return SdfLayer::FindRelativeToLayer(anchor, identifier, args);
}

static SdfLayerRefPtr
_FindOrOpenRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& identifier,
    const nb::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    return SdfLayer::FindOrOpenRelativeToLayer(anchor, identifier, args);
}

using Py_SdfLayerTraversalFunctionSig = void(const SdfPath&);

// Just for testing purposes.
static void _TestTakeOwnership(SdfLayerRefPtr layerRef)
{
    // do nothing
}

} // anonymous namespace 

void wrapLayer(nb::module_& m)
{
    typedef SdfLayer       This;
    typedef SdfLayerHandle ThisHandle;

    TfPyFunctionFromPython<::Py_SdfLayerTraversalFunctionSig>();

    Sdf_SubLayerOffsetsProxy::Wrap(m);

    m.def("ComputeAssetPathRelativeToLayer", &SdfComputeAssetPathRelativeToLayer,
        nb::arg("anchor"),
        nb::arg("assetPath"));

    m.def("_TestTakeOwnership", &_TestTakeOwnership);

    nb::class_<This> s(m, "Layer", nb::is_weak_referenceable(), nb::dynamic_attr());
    s
        .def(TfNbRefAndWeakPtr())

        .def("__repr__", &_Repr)

        .def("GetFileFormat", &This::GetFileFormat,
             nb::rv_policy::copy)
        .def("GetFileFormatArguments", &This::GetFileFormatArguments,
             nb::rv_policy::copy)

        .def_static("CreateNew", &_CreateNew,
             nb::arg("identifier"),
             nb::arg("args") = nb::dict())

        .def_static("CreateAnonymous", 
             (SdfLayerRefPtr (*)(const std::string &,
                                 const nb::dict &))
             &_CreateAnonymous,
             nb::arg("tag") = std::string(),
             nb::arg("args") = nb::dict())
        .def_static("CreateAnonymous",
             (SdfLayerRefPtr (*)(const std::string &,
                                 const SdfFileFormatConstPtr &,
                                 const nb::dict &))
             &_CreateAnonymous,
             nb::arg("tag"), 
             nb::arg("format"), 
             nb::arg("args") = nb::dict())

        .def_static("New", &_New,
             nb::arg("fileFormat"),
             nb::arg("identifier"),
             nb::arg("args") = nb::dict())

        .def_static("FindOrOpen", &_FindOrOpen,
             nb::arg("identifier"),
             nb::arg("args") = nb::dict())

        .def_static("FindOrOpenRelativeToLayer", &_FindOrOpenRelativeToLayer,
             nb::arg("anchor"),
             nb::arg("identifier"),
             nb::arg("args") = nb::dict())

        .def_static("OpenAsAnonymous", This::OpenAsAnonymous,
             nb::arg("filePath") = std::string(),
             nb::arg("metadataOnly") = false,
             nb::arg("tag") = std::string())

        .def("Save", &This::Save,
             nb::arg("force") = false)
        .def("Export", &_Export,
             nb::arg("filename"),
             nb::arg("comment") = std::string(),
             nb::arg("args") = nb::dict())

        .def("ExportToString", &_ExportToString, 
             "Returns the string representation of the layer.\n")

        .def("ImportFromString",
             &SdfLayer::ImportFromString)

        .def("Clear", &This::Clear)

        .def("Reload", &This::Reload,
             nb::arg("force") = false)

        .def_static("ReloadLayers", &This::ReloadLayers,
             nb::arg("layers"),
             nb::arg("force") = false)

        .def("Import", &This::Import)

        .def("TransferContent", &SdfLayer::TransferContent)

        .def("StreamsData", &This::StreamsData)

        .def("IsDetached", &This::IsDetached)

        .def_prop_ro("empty", &This::IsEmpty)

        .def_prop_ro("dirty", &This::IsDirty)

        .def_prop_ro("anonymous", &This::IsAnonymous)

        .def_static("IsAnonymousLayerIdentifier",
             &This::IsAnonymousLayerIdentifier)

        .def_static("GetDisplayNameFromIdentifier",
             &This::GetDisplayNameFromIdentifier)

        .def_static("SplitIdentifier", &_SplitIdentifier)

        .def_static("CreateIdentifier", &This::CreateIdentifier)

        .def_prop_rw("identifier",
            [](const This& self) { return self.GetIdentifier(); },
            &This::SetIdentifier,
            "The layer's identifier.")

        .def_prop_ro("resolvedPath",
            &This::GetResolvedPath,
            nb::rv_policy::copy,
            "The layer's resolved path.")

        .def_prop_ro("realPath",
            &This::GetRealPath,
            nb::rv_policy::copy,
            "The layer's resolved path.")

        .def_prop_ro("fileExtension", &This::GetFileExtension,
            "The layer's file extension.")

        .def_prop_ro("version",
            &This::GetVersion,
            nb::rv_policy::copy,
            "The layer's version.")

        .def_prop_ro("repositoryPath",
            &This::GetRepositoryPath,
            nb::rv_policy::copy,
            "The layer's associated repository path")

        .def("GetAssetName", &This::GetAssetName,
             nb::rv_policy::copy)

        .def("GetAssetInfo", &This::GetAssetInfo,
             nb::rv_policy::copy)

        .def("GetDisplayName", &This::GetDisplayName)

        .def("UpdateAssetInfo", &This::UpdateAssetInfo)

        .def("ComputeAbsolutePath", &This::ComputeAbsolutePath)

        .def("ScheduleRemoveIfInert", &This::ScheduleRemoveIfInert)

        .def("RemovePropertyIfHasOnlyRequiredFields",
             &This::RemovePropertyIfHasOnlyRequiredFields)

        .def("RemoveInertSceneDescription", &This::RemoveInertSceneDescription)

        .def("UpdateExternalReference", &This::UpdateExternalReference)

        .def("UpdateCompositionAssetDependency", 
             &This::UpdateCompositionAssetDependency)

        .def_static("SetDetachedLayerRules", &This::SetDetachedLayerRules)

        .def_static("GetDetachedLayerRules", &This::GetDetachedLayerRules,
             nb::rv_policy::copy)

        .def_static("IsIncludedByDetachedLayerRules", 
             &This::IsIncludedByDetachedLayerRules)

        .def("SetMuted", &This::SetMuted)

        .def("IsMuted", &_WrapIsMuted)

        .def_static("AddToMutedLayers", &This::AddToMutedLayers)

        .def_static("RemoveFromMutedLayers", &This::RemoveFromMutedLayers)

        .def_static("GetMutedLayers",
             []() { return TfNbSequenceToList(This::GetMutedLayers()); },
             "Return list of muted layers.\n")

        .def("Traverse",
             &This::Traverse,
             nb::arg("path"), nb::arg("func"))

        .def_prop_rw("colorConfiguration",
            &This::GetColorConfiguration,
            &This::SetColorConfiguration,
            "The color configuration asset-path of this layer.")

        .def("HasColorConfiguration", &This::HasColorConfiguration)
        .def("ClearColorConfiguration", &This::ClearColorConfiguration)

        .def_prop_rw("colorManagementSystem",
            &This::GetColorManagementSystem,
            &This::SetColorManagementSystem,
            "The name of the color management system used to interpret the "
            "colorConfiguration asset.")

        .def("HasColorManagementSystem", &This::HasColorManagementSystem)
        .def("ClearColorManagementSystem", &This::ClearColorManagementSystem)

        .def_prop_rw("comment",
            &This::GetComment,
            &This::SetComment,
            "The layer's comment string.")

        .def_prop_rw("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "The layer's documentation string.")

        .def_prop_rw("defaultPrim",
                      &This::GetDefaultPrim,
                      &This::SetDefaultPrim,
                      "The layer's default reference target token.")
        .def("GetDefaultPrimAsPath",
             &This::GetDefaultPrimAsPath)
        .def("HasDefaultPrim",
             &This::HasDefaultPrim)
        .def("ClearDefaultPrim",
             &This::ClearDefaultPrim)
        .def_static("ConvertDefaultPrimTokenToPath",
             &This::ConvertDefaultPrimTokenToPath)
        .def_static("ConvertDefaultPrimPathToToken",
             &This::ConvertDefaultPrimPathToToken)

        .def_prop_rw("customLayerData",
           &This::GetCustomLayerData,
           &This::SetCustomLayerData,
           "The customLayerData dictionary associated with this layer.")

        .def("HasCustomLayerData", &This::HasCustomLayerData)
        .def("ClearCustomLayerData", &This::ClearCustomLayerData)

        .def_prop_rw("expressionVariables",
           &This::GetExpressionVariables,
           &This::SetExpressionVariables,
           "The expressionVariables dictionary associated with this layer.")

        .def("HasExpressionVariables", &This::HasExpressionVariables)
        .def("ClearExpressionVariables", &This::ClearExpressionVariables)

        .def_prop_rw("startTimeCode",
            &This::GetStartTimeCode,
            &This::SetStartTimeCode,
            "The start timeCode of this layer.\n\n" 
            "The start timeCode of a layer is not a hard limit, but is \n"
            "more of a hint.  A layer's time-varying content is not limited to \n"
            "the timeCode range of the layer." )

        .def("HasStartTimeCode", &This::HasStartTimeCode)
        .def("ClearStartTimeCode", &This::ClearStartTimeCode)

        .def_prop_rw("endTimeCode",
            &This::GetEndTimeCode,
            &This::SetEndTimeCode,
            "The end timeCode of this layer.\n\n"
            "The end timeCode of a layer is not a hard limit, but is \n"
            "more of a hint. A layer's time-varying content is not limited to\n"
            "the timeCode range of the layer." )

        .def("HasEndTimeCode", &This::HasEndTimeCode)
        .def("ClearEndTimeCode", &This::ClearEndTimeCode)

        .def_prop_rw("timeCodesPerSecond",
            &This::GetTimeCodesPerSecond,
            &This::SetTimeCodesPerSecond,
            "The timeCodes per second used in this layer.")

        .def("HasTimeCodesPerSecond", &This::HasTimeCodesPerSecond)
        .def("ClearTimeCodesPerSecond", &This::ClearTimeCodesPerSecond)

        .def_prop_rw("framesPerSecond",
            &This::GetFramesPerSecond,
            &This::SetFramesPerSecond,
            "The frames per second used in this layer.")

        .def("HasFramesPerSecond", &This::HasFramesPerSecond)
        .def("ClearFramesPerSecond", &This::ClearFramesPerSecond)

        .def_prop_rw("framePrecision",
            &This::GetFramePrecision,
            &This::SetFramePrecision,
            "The number of digits of precision used in times in this layer.")

        .def("HasFramePrecision", &This::HasFramePrecision)
        .def("ClearFramePrecision", &This::ClearFramePrecision)

        .def_prop_rw("owner",
            &This::GetOwner,
            &This::SetOwner,
            "The owner of this layer.")

        .def("HasOwner", &This::HasOwner)
        .def("ClearOwner", &This::ClearOwner)

        .def_prop_rw("sessionOwner",
            &This::GetSessionOwner,
            &This::SetSessionOwner,
            "The session owner of this layer. Only intended for use with "
            "session layers.")

        .def("HasSessionOwner", &This::HasSessionOwner)
        .def("ClearSessionOwner", &This::ClearSessionOwner)

        .def_prop_rw("hasOwnedSubLayers",
            &This::GetHasOwnedSubLayers,
            &This::SetHasOwnedSubLayers,
            "Whether this layer's sub layers are expected to have owners.")

        .def_prop_ro("pseudoRoot",
            &This::GetPseudoRoot,
            "The pseudo-root of the layer.")

        .def_prop_ro("rootPrims",
            &_WrapGetRootPrims,
            "The root prims of this layer, as an ordered dictionary.\n\n"
            "The prims may be accessed by index or by name.\n"
            "Although this property claims it is read only, you can modify "
            "the contents of this dictionary to add, remove, or reorder "
            "the contents.")

        .def_prop_rw("rootPrimOrder", 
            &This::GetRootPrimOrder,
            &This::SetRootPrimOrder,
            "Get/set the list of root prim names for this layer's 'reorder "
            "rootPrims' statement.")

        .def("GetObjectAtPath", &This::GetObjectAtPath)
        .def("GetPrimAtPath", &This::GetPrimAtPath)
        .def("GetPropertyAtPath", &This::GetPropertyAtPath)
        .def("GetAttributeAtPath", &This::GetAttributeAtPath)
        .def("GetRelationshipAtPath", &This::GetRelationshipAtPath)

        .def("SetPermissionToEdit", &This::SetPermissionToEdit)
        .def("SetPermissionToSave", &This::SetPermissionToSave)

        .def("CanApply", &_CanApplyNamespaceEdit)
        .def("Apply", &This::Apply)

        .def_prop_rw("subLayerPaths",
            &This::GetSubLayerPaths,
            &This::SetSubLayerPaths,
            "The sublayer paths of this layer, as a list.  Although this "
            "property is claimed to be read only, you can modify the contents "
            "of this list.")

        .def_prop_ro("subLayerOffsets",
            &_WrapGetSubLayerOffsets,
            "The sublayer offsets of this layer, as a list.  Although this "
            "property is claimed to be read only, you can modify the contents "
            "of this list by assigning new layer offsets to specific indices.")

        .def_prop_rw("relocates", 
            &This::GetRelocates,
            &This::SetRelocates)
        .def("HasRelocates", &This::HasRelocates)
        .def("ClearRelocates", &This::ClearRelocates)

        .def_static("GetLoadedLayers",
            []() { return TfNbSequenceToList(This::GetLoadedLayers()); },
            "Return list of loaded layers.\n")

        .def_static("Find", &_Find,
            nb::arg("identifier"),
            nb::arg("args") = nb::dict(),
            "Find(filename) -> LayerPtr\n\n"
            "filename : string\n\n"
            "Returns the open layer with the given filename, or None.  "
            "Note that this is a static class method.")

        .def_static("FindRelativeToLayer", &_FindRelativeToLayer,
            nb::arg("anchor"),
            nb::arg("assetPath"),
            nb::arg("args") = nb::dict(),
            "Returns the open layer with the given filename, or None.  "
            "If the filename is a relative path then it's found relative "
            "to the given layer.  "
            "Note that this is a static class method.")

        .def_static("DumpLayerInfo", &This::DumpLayerInfo,
            "Debug helper to examine content of the current layer registry and\n"
            "the asset/real path of all layers in the registry.")

        .def("GetExternalReferences",
            [](const This& self) {
                return TfNbSequenceToTuple(self.GetExternalReferences());
            },
            "Return a list of asset paths for\n"
            "this layer.")

        .def_prop_ro("externalReferences",
            [](const This& self) {
                return TfNbSequenceToList(self.GetExternalReferences());
            },
            "Return unique list of asset paths of external references for\n"
            "given layer.")

        .def("GetCompositionAssetDependencies",
             [](const This& self) {
                 return TfNbSequenceToList(self.GetCompositionAssetDependencies());
             })

        .def("GetExternalAssetDependencies",
             [](const This& self) {
                 return TfNbSequenceToList(self.GetExternalAssetDependencies());
             })

        .def_prop_ro("permissionToSave", &This::PermissionToSave, 
              "Return true if permitted to be saved, false otherwise.\n")

        .def_prop_ro("permissionToEdit", &This::PermissionToEdit, 
              "Return true if permitted to be edited (modified), false otherwise.\n")

        .def("ApplyRootPrimOrder", &_ApplyRootPrimOrder)

        .def("_WriteDataFile", &SdfLayer::WriteDataFile)

        .def("ListAllTimeSamples",
             [](const This& self) {
                 return TfNbSequenceToList(self.ListAllTimeSamples());
             })
        .def("ListTimeSamplesForPath", &_ListTimeSamplesForPath)
        .def("GetNumTimeSamplesForPath", &_GetNumTimeSamplesForPath)
        .def("GetBracketingTimeSamples",
             &_GetBracketingTimeSamples)
        .def("GetBracketingTimeSamplesForPath",
             &_GetBracketingTimeSamplesForPath)
        .def("GetPreviousTimeSampleForPath",
             &_GetPreviousTimeSampleForPath)
        .def("QueryTimeSample",
             &_QueryTimeSample)
        .def("SetTimeSample", &_SetTimeSample)
        .def("EraseTimeSample", &_EraseTimeSample)
        ;

    s.attr("ColorConfigurationKey") = nb::cast(SdfFieldKeys->ColorConfiguration);
    s.attr("ColorManagementSystemKey") = nb::cast(SdfFieldKeys->ColorManagementSystem);
    s.attr("CommentKey") = nb::cast(SdfFieldKeys->Comment);
    s.attr("DocumentationKey") = nb::cast(SdfFieldKeys->Documentation);
    s.attr("HasOwnedSubLayers") = nb::cast(SdfFieldKeys->HasOwnedSubLayers);
    s.attr("StartFrameKey") = nb::cast(SdfFieldKeys->StartFrame);
    s.attr("EndFrameKey") = nb::cast(SdfFieldKeys->EndFrame);
    s.attr("StartTimeCodeKey") = nb::cast(SdfFieldKeys->StartTimeCode);
    s.attr("EndTimeCodeKey") = nb::cast(SdfFieldKeys->EndTimeCode);
    s.attr("FramesPerSecondKey") = nb::cast(SdfFieldKeys->FramesPerSecond);
    s.attr("FramePrecisionKey") = nb::cast(SdfFieldKeys->FramePrecision);
    s.attr("OwnerKey") = nb::cast(SdfFieldKeys->Owner);
    s.attr("LayerRelocatesKey") = nb::cast(SdfFieldKeys->LayerRelocates);
    s.attr("SessionOwnerKey") = nb::cast(SdfFieldKeys->SessionOwner);
    s.attr("TimeCodesPerSecondKey") = nb::cast(SdfFieldKeys->TimeCodesPerSecond);

    {
        using Rules = SdfLayer::DetachedLayerRules;
        nb::class_<Rules>(s, "DetachedLayerRules")
            .def(nb::init<>())

            .def("IncludeAll", &Rules::IncludeAll,
                 nb::rv_policy::copy)
            .def("Include", &Rules::Include,
                 nb::rv_policy::copy)
            .def("Exclude", &Rules::Exclude,
                 nb::rv_policy::copy)

            .def("IncludedAll", &Rules::IncludedAll)
            .def("GetIncluded", &Rules::GetIncluded,
                 nb::rv_policy::copy)
            .def("GetExcluded", &Rules::GetExcluded,
                 nb::rv_policy::copy)

            .def("IsIncluded", &Rules::IsIncluded)
            ;
    }

    TfPyContainerConversions::from_python_sequence<
        SdfLayerHandleSet, TfPyContainerConversions::set_policy>();

    TfPyContainerConversions::from_python_sequence<
        SdfLayerHandleVector,
        TfPyContainerConversions::variable_capacity_policy>();

}
