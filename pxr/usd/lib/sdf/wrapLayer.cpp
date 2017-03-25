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
/// \file wrapLayer.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
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

#include <boost/python.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;

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
        // Wrap as soon as the first instance is constructed.
        TfPyWrapOnce<Sdf_SubLayerOffsetsProxy>(&_Wrap);
    }

private:
    static void _Wrap()
    {
        using namespace boost::python;

        class_<Sdf_SubLayerOffsetsProxy>("SubLayerOffsetsProxy", no_init)
            .def("__len__", &This::_GetSize)
            .def("__eq__", &This::_EqVec)
            .def("__ne__", &This::_NeVec)
            .def("__eq__", &This::_EqProxy)
            .def("__ne__", &This::_NeProxy)
            .def("__getitem__", &This::_GetItemByIndex)
            .def("__getitem__", &This::_GetItemByPath)
            .def("__repr__", &This::_GetRepr)
            .def("count", &This::_Count)
            .def("copy", &This::_GetValues,
                 return_value_policy<TfPySequenceToList>())
            .def("index", &This::_FindIndexForValue)
            .def("__setitem__", &This::_SetItemByIndex)
            .def("__setitem__", &This::_SetItemByPath)
            ;
    }

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

    int _EqVec(const SdfLayerOffsetVector& rhs)
    {
        return (_GetValues() == rhs) ? 1 : 0;
    }

    int _NeVec(const SdfLayerOffsetVector& rhs)
    {
        return (_EqVec(rhs) ? 0 : 1);
    }

    int _EqProxy(const This& rhs)
    {
        SdfLayerOffsetVector values    =     _GetValues();
        SdfLayerOffsetVector rhsValues = rhs._GetValues();
        return (values == rhsValues) ? 1 : 0;
    }

    int _NeProxy(const This& rhs)
    {
        return (_EqProxy(rhs) ? 0 : 1);
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
    const boost::python::dict& dict,
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
_Repr(const SdfLayerHandle &self)
{
    return TF_PY_REPR_PREFIX + "Find(" + TfPyRepr(self->GetIdentifier()) + ")";
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
    const boost::python::dict& dict)
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

static std::set<double>
_ListTimeSamplesForPath(const SdfLayerHandle& layer, const SdfPath& path)
{
    return layer->ListTimeSamplesForPath(path);
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

static tuple
_GetBracketingTimeSamples(const SdfLayerHandle & layer, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = layer->GetBracketingTimeSamples(time, &tLower, &tUpper);
    return boost::python::make_tuple(found, tLower, tUpper);
}

static tuple
_GetBracketingTimeSamplesForPath(const SdfLayerHandle & layer,
                                 const SdfPath & path, double time)
{
    double tLower = 0, tUpper = 0;
    bool found = layer->GetBracketingTimeSamplesForPath(path, time,
                                                        &tLower, &tUpper);
    return boost::python::make_tuple(found, tLower, tUpper);
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

static boost::python::tuple
_SplitIdentifier(const std::string& identifier)
{
    std::string layerPath;
    SdfLayer::FileFormatArguments args;
    SdfLayer::SplitIdentifier(identifier, &layerPath, &args);
    return boost::python::make_tuple(layerPath, args);
}

static
object
_CanApplyNamespaceEdit(
    const SdfLayerHandle& x,
    const SdfBatchNamespaceEdit& edit)
{
    SdfNamespaceEditDetailVector details;
    SdfNamespaceEditDetail::Result result = x->CanApply(edit, &details);
    if (result != SdfNamespaceEditDetail::Okay) {
        return make_tuple(object(false), object(details));
    }
    else {
        return object(true);
    }
}

static SdfLayerRefPtr
_CreateNew(
    const std::string& identifier,
    const std::string& realPath = std::string(),
    const boost::python::dict& dict = boost::python::dict())
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::CreateNew(identifier, realPath, args);
}

static SdfLayerRefPtr
_New(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier,
    const std::string& realPath = std::string(),
    const boost::python::dict& dict = boost::python::dict())
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::New(fileFormat, identifier, realPath, args);
}

static SdfLayerRefPtr
_FindOrOpen(
    const std::string& identifier,
    const boost::python::dict& dict)
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
    const boost::python::dict& dict)
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
    const std::string& assetPath,
    const boost::python::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    return SdfLayer::FindRelativeToLayer(anchor, assetPath, args);
}

static SdfLayerRefPtr
_FindOrOpenRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& layerPath,
    const boost::python::dict& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    std::string mutableLayerPath(layerPath);
    return SdfFindOrOpenRelativeToLayer(anchor, &mutableLayerPath, args);
}

} // anonymous namespace 

void wrapLayer()
{
    typedef SdfLayer       This;
    typedef SdfLayerHandle ThisHandle;

    def("FindOrOpenRelativeToLayer", &_FindOrOpenRelativeToLayer,
         ( arg("anchor"),
           arg("layerPath"),
           arg("args") = boost::python::dict()),
         return_value_policy<TfPyRefPtrFactory<ThisHandle> >());

    scope s = class_<This,
                     ThisHandle,
                     bases<SdfLayerBase>,
                     boost::noncopyable>("Layer", no_init)

        .def(TfPyRefAndWeakPtr())

        .def("__repr__", _Repr)

        .def("CreateNew", &_CreateNew,
             ( arg("identifier"),
               arg("realPath") = std::string(),
               arg("args") = boost::python::dict()),
             return_value_policy<TfPyRefPtrFactory<ThisHandle> >())
        .staticmethod("CreateNew")

        .def("CreateAnonymous", This::CreateAnonymous,
             return_value_policy<TfPyRefPtrFactory<ThisHandle> >(),
             ( arg("tag") = std::string() ))
        .staticmethod("CreateAnonymous")

        .def("New", &_New,
             ( arg("fileFormat"),
               arg("identifier"),
               arg("realPath") = std::string(),
               arg("args") = boost::python::dict()),
             return_value_policy<TfPyRefPtrFactory<ThisHandle> >())
        .staticmethod("New")

        .def("FindOrOpen", &_FindOrOpen,
             ( arg("identifier"),
               arg("args") = boost::python::dict()),
             return_value_policy<TfPyRefPtrFactory<ThisHandle> >())
        .staticmethod("FindOrOpen")

        .def("OpenAsAnonymous", This::OpenAsAnonymous,
             ( arg("filePath") = std::string(),
               arg("metadataOnly") = false ),
             return_value_policy<TfPyRefPtrFactory<ThisHandle> >())
        .staticmethod("OpenAsAnonymous")

        .def("Save", &This::Save)
        .def("Export", &_Export,
             ( arg("filename"),
               arg("comment") = std::string(),
               arg("args") = boost::python::dict()))

        .def("ExportToString", &_ExportToString, 
             "Returns the string representation of the layer.\n")

        .def("ImportFromString",
             &SdfLayer::ImportFromString)

        .def("Clear", &This::Clear)

        .def("Reload", &This::Reload,
             ( arg("force") = false ))

        .def("ReloadLayers", &This::ReloadLayers,
             (arg("force") = false))
        .staticmethod("ReloadLayers")

        .def("Import", &This::Import)

        .def("TransferContent", &SdfLayer::TransferContent)

        .add_property("empty", &This::IsEmpty)

        .add_property("dirty", &This::IsDirty)

        .add_property("anonymous", &This::IsAnonymous)

        .def("IsAnonymousLayerIdentifier",
             &This::IsAnonymousLayerIdentifier)
        .staticmethod("IsAnonymousLayerIdentifier")

        .def("GetDisplayNameFromIdentifier",
             &This::GetDisplayNameFromIdentifier)
        .staticmethod("GetDisplayNameFromIdentifier")

        .def("SplitIdentifier", &_SplitIdentifier)
        .staticmethod("SplitIdentifier")

        .def("CreateIdentifier", &This::CreateIdentifier)
        .staticmethod("CreateIdentifier")

        .add_property("identifier",
            make_function(&This::GetIdentifier,
                return_value_policy<return_by_value>()),
            &This::SetIdentifier,
            "The layer's identifier.")

        .add_property("realPath",
            make_function(&This::GetRealPath,
                return_value_policy<return_by_value>()),
            "The layer's canonical full path. This path is guaranteed to\n"
            "be valid.")

        .add_property("fileExtension", &This::GetFileExtension,
            "The layer's file extension.")

        .add_property("version",
            make_function(&This::GetVersion,
                return_value_policy<return_by_value>()),
            "The layer's version.")

        .add_property("repositoryPath",
            make_function(&This::GetRepositoryPath,
                return_value_policy<return_by_value>()),
            "The layer's associated repository path")

        .def("GetAssetName", &This::GetAssetName,
             return_value_policy<return_by_value>())

        .def("GetAssetInfo", &This::GetAssetInfo,
             return_value_policy<return_by_value>())

        .def("GetDisplayName", &This::GetDisplayName)

        .def("UpdateAssetInfo", &This::UpdateAssetInfo,
             ( arg("fileVersion") = std::string() ))

        .def("ComputeAbsolutePath", &This::ComputeAbsolutePath)

        .def("ScheduleRemoveIfInert", &This::ScheduleRemoveIfInert)

        .def("RemoveInertSceneDescription", &This::RemoveInertSceneDescription)

        .def("UpdateExternalReference", &This::UpdateExternalReference)

        .def("SetMuted", &This::SetMuted)

        .def("IsMuted", &_WrapIsMuted)

        .def("AddToMutedLayers", &This::AddToMutedLayers)
             .staticmethod("AddToMutedLayers")

        .def("RemoveFromMutedLayers", &This::RemoveFromMutedLayers)
             .staticmethod("RemoveFromMutedLayers")

        .def("GetMutedLayers",
                make_function(&This::GetMutedLayers, 
                      return_value_policy<TfPySequenceToList>()), 
             "Return list of muted layers.\n")
             .staticmethod("GetMutedLayers")

        .add_property("comment",
            &This::GetComment,
            &This::SetComment,
            "The layer's comment string.")

        .add_property("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "The layer's documentation string.")

        .add_property("defaultPrim",
                      &This::GetDefaultPrim,
                      &This::SetDefaultPrim,
                      "The layer's default reference target token.")
        .def("HasDefaultPrim",
             &This::HasDefaultPrim)
        .def("ClearDefaultPrim",
             &This::ClearDefaultPrim)

        .add_property("customLayerData",
           &This::GetCustomLayerData,
           &This::SetCustomLayerData,
           "The customLayerData dictionary associated with this layer.")

        .def("HasCustomLayerData", &This::HasCustomLayerData)
        .def("ClearCustomLayerData", &This::ClearCustomLayerData)

        .add_property("startTimeCode",
            &This::GetStartTimeCode,
            &This::SetStartTimeCode,
            "The start timeCode of this layer.\n\n" 
            "The start timeCode of a layer is not a hard limit, but is \n"
            "more of a hint.  A layer's time-varying content is not limited to \n"
            "the timeCode range of the layer." )

        .def("HasStartTimeCode", &This::HasStartTimeCode)
        .def("ClearStartTimeCode", &This::ClearStartTimeCode)

        .add_property("endTimeCode",
            &This::GetEndTimeCode,
            &This::SetEndTimeCode,
            "The end timeCode of this layer.\n\n"
            "The end timeCode of a layer is not a hard limit, but is \n"
            "more of a hint. A layer's time-varying content is not limited to\n"
            "the timeCode range of the layer." )

        .def("HasEndTimeCode", &This::HasEndTimeCode)
        .def("ClearEndTimeCode", &This::ClearEndTimeCode)

        .add_property("timeCodesPerSecond",
            &This::GetTimeCodesPerSecond,
            &This::SetTimeCodesPerSecond,
            "The timeCodes per second used in this layer.")

        .def("HasTimeCodesPerSecond", &This::HasTimeCodesPerSecond)
        .def("ClearTimeCodesPerSecond", &This::ClearTimeCodesPerSecond)

        .add_property("framesPerSecond",
            &This::GetFramesPerSecond,
            &This::SetFramesPerSecond,
            "The frames per second used in this layer.")

        .def("HasFramesPerSecond", &This::HasFramesPerSecond)
        .def("ClearFramesPerSecond", &This::ClearFramesPerSecond)

        .add_property("framePrecision",
            &This::GetFramePrecision,
            &This::SetFramePrecision,
            "The number of digits of precision used in times in this layer.")

        .def("HasFramePrecision", &This::HasFramePrecision)
        .def("ClearFramePrecision", &This::ClearFramePrecision)

        .add_property("owner",
            &This::GetOwner,
            &This::SetOwner,
            "The owner of this layer.")

        .def("HasOwner", &This::HasOwner)
        .def("ClearOwner", &This::ClearOwner)

        .add_property("sessionOwner",
            &This::GetSessionOwner,
            &This::SetSessionOwner,
            "The session owner of this layer. Only intended for use with "
            "session layers.")

        .def("HasSessionOwner", &This::HasSessionOwner)
        .def("ClearSessionOwner", &This::ClearSessionOwner)

        .add_property("hasOwnedSubLayers",
            &This::GetHasOwnedSubLayers,
            &This::SetHasOwnedSubLayers,
            "Whether this layer's sub layers are expected to have owners.")

        .add_property("pseudoRoot",
            &This::GetPseudoRoot,
            "The pseudo-root of the layer.")

        .add_property("rootPrims",
            &_WrapGetRootPrims,
            "The root prims of this layer, as an ordered dictionary.\n\n"
            "The prims may be accessed by index or by name.\n"
            "Although this property claims it is read only, you can modify "
            "the contents of this dictionary to add, remove, or reorder "
            "the contents.")

        .add_property("rootPrimOrder", 
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

        .add_property("subLayerPaths",
            &This::GetSubLayerPaths,
            &This::SetSubLayerPaths,
            "The sublayer paths of this layer, as a list.  Although this "
            "property is claimed to be read only, you can modify the contents "
            "of this list.")

        .add_property("subLayerOffsets",
            &_WrapGetSubLayerOffsets,
            "The sublayer offsets of this layer, as a list.  Although this "
            "property is claimed to be read only, you can modify the contents "
            "of this list by assigning new layer offsets to specific indices.")

        .def("GetLoadedLayers",
            make_function(&This::GetLoadedLayers, 
                          return_value_policy<TfPySequenceToList>()), 
            "Return list of loaded layers.\n")
        .staticmethod("GetLoadedLayers")

        .def("Find", &_Find,
            ( arg("identifier"),
              arg("args") = boost::python::dict()),
            "Find(filename) -> LayerPtr\n\n"
            "filename : string\n\n"
            "Returns the open layer with the given filename, or None.  "
            "Note that this is a static class method.")
        .staticmethod("Find")

        .def("FindRelativeToLayer", &_FindRelativeToLayer,
            ( arg("anchor"),
              arg("assetPath"),
              arg("args") = boost::python::dict()),
            "Returns the open layer with the given filename, or None.  "
            "If the filename is a relative path then it's found relative "
            "to the given layer.  "
            "Note that this is a static class method.")
        .staticmethod("FindRelativeToLayer")

        .def("DumpLayerInfo", &This::DumpLayerInfo,
            "Debug helper to examine content of the current layer registry and\n"
            "the asset/real path of all layers in the registry.")
        .staticmethod("DumpLayerInfo")

        .def("GetExternalReferences", 
            make_function(&This::GetExternalReferences,
                          return_value_policy<TfPySequenceToTuple>()), 
            "Return a list of asset paths for\n"
            "this layer.")

        .add_property("externalReferences",
            make_function(&This::GetExternalReferences, 
                          return_value_policy<TfPySequenceToList>()),
            "Return unique list of asset paths of external references for\n"
            "given layer.")

        .add_property("permissionToSave", &This::PermissionToSave, 
              "Return true if permitted to be saved, false otherwise.\n")

        .add_property("permissionToEdit", &This::PermissionToEdit, 
              "Return true if permitted to be edited (modified), false otherwise.\n")

        .def("ApplyRootPrimOrder", &_ApplyRootPrimOrder,
                 return_value_policy<TfPySequenceToList>())

        .setattr("CommentKey", SdfFieldKeys->Comment)
        .setattr("DocumentationKey", SdfFieldKeys->Documentation)
        .setattr("HasOwnedSubLayers", SdfFieldKeys->HasOwnedSubLayers)
        .setattr("StartFrameKey", SdfFieldKeys->StartFrame)
        .setattr("EndFrameKey", SdfFieldKeys->EndFrame)
        .setattr("StartTimeCodeKey", SdfFieldKeys->StartTimeCode)
        .setattr("EndTimeCodeKey", SdfFieldKeys->EndTimeCode)
        .setattr("FramesPerSecondKey", SdfFieldKeys->FramesPerSecond)
        .setattr("FramePrecisionKey", SdfFieldKeys->FramePrecision)
        .setattr("OwnerKey", SdfFieldKeys->Owner)
        .setattr("SessionOwnerKey", SdfFieldKeys->SessionOwner)
        .setattr("TimeCodesPerSecondKey", SdfFieldKeys->TimeCodesPerSecond)

        .def("_WriteDataFile", &SdfLayer::WriteDataFile)

        .def("ListAllTimeSamples", &SdfLayer::ListAllTimeSamples,
             return_value_policy<TfPySequenceToList>())
        .def("ListTimeSamplesForPath", &_ListTimeSamplesForPath,
             return_value_policy<TfPySequenceToList>())
        .def("GetNumTimeSamplesForPath", &_GetNumTimeSamplesForPath)
        .def("GetBracketingTimeSamples",
             &_GetBracketingTimeSamples)
        .def("GetBracketingTimeSamplesForPath",
             &_GetBracketingTimeSamplesForPath)
        .def("QueryTimeSample",
             &_QueryTimeSample)
        .def("SetTimeSample", &_SetTimeSample)
        .def("EraseTimeSample", &_EraseTimeSample)
        ;

    TfPyContainerConversions::from_python_sequence<
        SdfLayerHandleSet, TfPyContainerConversions::set_policy>();

    TfPyContainerConversions::from_python_sequence<
        SdfLayerHandleVector,
        TfPyContainerConversions::variable_capacity_policy>();

}

TF_REFPTR_CONST_VOLATILE_GET(SdfLayer)
