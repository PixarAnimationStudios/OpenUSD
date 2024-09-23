//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/object.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/ostreamMethods.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/extract.hpp"

#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static TfPyObjWrapper
_GetMetadata(const UsdObject &self, const TfToken &key)
{
    VtValue result;
    self.GetMetadata(key, &result);
    return UsdVtValueToPython(result);
}

static TfPyObjWrapper
_GetMetadataByDictKey(
    const UsdObject &self, const TfToken &key, const TfToken &keyPath)
{
    VtValue result;
    self.GetMetadataByDictKey(key, keyPath, &result);
    return UsdVtValueToPython(result);
}


static bool _SetMetadata(const UsdObject &self, const TfToken& key,
                         object obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, /*keyPath*/TfToken(), obj, &value) &&
        self.SetMetadata(key, value);
}

static bool _SetMetadataByDictKey(const UsdObject &self, const TfToken& key,
                                  const TfToken &keyPath, object obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, keyPath, obj, &value) &&
        self.SetMetadataByDictKey(key, keyPath, value);
}

static TfPyObjWrapper _GetCustomData(const UsdObject &self) {
    return UsdVtValueToPython(VtValue(self.GetCustomData()));
}

static TfPyObjWrapper _GetCustomDataByKey(
    const UsdObject &self, const TfToken &keyPath) {
    return UsdVtValueToPython(VtValue(self.GetCustomDataByKey(keyPath)));
}

static void _SetCustomData(UsdObject &self, object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->CustomData, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetCustomData(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetCustomDataByKey(
    UsdObject &self, const TfToken &keyPath, object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->CustomData, keyPath, obj, &value)) {
        self.SetCustomDataByKey(keyPath, value);
    }
}

static TfPyObjWrapper _GetAssetInfo(const UsdObject &self) {
    return UsdVtValueToPython(VtValue(self.GetAssetInfo()));
}

static TfPyObjWrapper _GetAssetInfoByKey(
    const UsdObject &self, const TfToken &keyPath) {
    return UsdVtValueToPython(VtValue(self.GetAssetInfoByKey(keyPath)));
}

static void _SetAssetInfo(UsdObject &self, object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->AssetInfo, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetAssetInfo(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetAssetInfoByKey(
    UsdObject &self, const TfToken &keyPath, object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->AssetInfo, keyPath, obj, &value)) {
        self.SetAssetInfoByKey(keyPath, value);
    }
}

static size_t __hash__(const UsdObject &self) { return hash_value(self); }

// We override __getattribute__ for UsdObject to check object validity and raise
// an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdObject.
static object
__getattribute__(object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', if the
    // object's prim is valid, or if the attribute is one of a specific
    // inclusion list.
    if ((name[0] == '_' && name[1] == '_') ||
        extract<UsdObject &>(selfObj)().GetPrim().IsValid() ||
        strcmp(name, "IsValid") == 0 ||
        strcmp(name, "GetDescription") == 0 ||
        strcmp(name, "GetPrim") == 0 ||
        strcmp(name, "GetPath") == 0 ||
        strcmp(name, "GetPrimPath") == 0 ||
        strcmp(name, "IsPseudoRoot") == 0){
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed %s", TfPyRepr(selfObj).c_str()));
    }
    // Unreachable.
    return object();
}

} // anonymous namespace 

void wrapUsdObject()
{
    class_<UsdObject> clsObj("Object");
    clsObj
        .def(Usd_ObjectSubclass())
        .def("IsValid", &UsdObject::IsValid)

        .def(self == self)
        .def(self != self)
        .def(!self)
        .def("__hash__", __hash__)

        .def("GetStage", &UsdObject::GetStage)
        .def("GetPath", &UsdObject::GetPath)
        .def("GetPrimPath", &UsdObject::GetPrimPath,
             return_value_policy<return_by_value>())
        .def("GetPrim", &UsdObject::GetPrim)
        .def("GetName", &UsdObject::GetName,
             return_value_policy<return_by_value>())
        .def("GetDescription", &UsdObject::GetDescription)

        .def("GetMetadata", _GetMetadata, arg("key"))
        .def("SetMetadata", _SetMetadata, (arg("key"), arg("value")))

        .def("ClearMetadata", &UsdObject::ClearMetadata, arg("key"))
        .def("HasMetadata", &UsdObject::HasMetadata, arg("key"))
        .def("HasAuthoredMetadata", &UsdObject::HasAuthoredMetadata, arg("key"))

        .def("GetMetadataByDictKey", _GetMetadataByDictKey,
             (arg("key"), arg("keyPath")))
        .def("SetMetadataByDictKey", _SetMetadataByDictKey,
             (arg("key"), arg("keyPath"), arg("value")))

        .def("ClearMetadataByDictKey", &UsdObject::ClearMetadataByDictKey,
             (arg("key"), arg("keyPath")))
        .def("HasMetadataDictKey", &UsdObject::HasMetadataDictKey,
             (arg("key"), arg("keyPath")))
        .def("HasAuthoredMetadataDictKey",
             &UsdObject::HasAuthoredMetadataDictKey,
             (arg("key"), arg("keyPath")))

        .def("GetAllMetadata", &UsdObject::GetAllMetadata,
             return_value_policy<TfPyMapToDictionary>())
        .def("GetAllAuthoredMetadata", &UsdObject::GetAllAuthoredMetadata,
             return_value_policy<TfPyMapToDictionary>())

        .def("IsHidden", &UsdObject::IsHidden)
        .def("SetHidden", &UsdObject::SetHidden, arg("hidden"))
        .def("ClearHidden", &UsdObject::ClearHidden)
        .def("HasAuthoredHidden", &UsdObject::HasAuthoredHidden)

        .def("GetCustomData", _GetCustomData)
        .def("GetCustomDataByKey", _GetCustomDataByKey,
             arg("keyPath"))
        .def("SetCustomData", _SetCustomData, arg("customData"))
        .def("SetCustomDataByKey", _SetCustomDataByKey,
             (arg("keyPath"), arg("value")))
        .def("ClearCustomData", &UsdObject::ClearCustomData)
        .def("ClearCustomDataByKey", &UsdObject::ClearCustomDataByKey,
             arg("keyPath"))

        .def("HasCustomData", &UsdObject::HasCustomData)
        .def("HasCustomDataKey", &UsdObject::HasCustomDataKey, arg("keyPath"))
        .def("HasAuthoredCustomData", &UsdObject::HasAuthoredCustomData)
        .def("HasAuthoredCustomDataKey", &UsdObject::HasAuthoredCustomDataKey,
             arg("keyPath"))

        .def("GetAssetInfo", _GetAssetInfo)
        .def("GetAssetInfoByKey", _GetAssetInfoByKey,
             arg("keyPath"))
        .def("SetAssetInfo", _SetAssetInfo, arg("assetInfo"))
        .def("SetAssetInfoByKey", _SetAssetInfoByKey,
             (arg("keyPath"), arg("value")))
        .def("ClearAssetInfo", &UsdObject::ClearAssetInfo)
        .def("ClearAssetInfoByKey", &UsdObject::ClearAssetInfoByKey,
             arg("keyPath"))

        .def("HasAssetInfo", &UsdObject::HasAssetInfo)
        .def("HasAssetInfoKey", &UsdObject::HasAssetInfoKey, arg("keyPath"))
        .def("HasAuthoredAssetInfo", &UsdObject::HasAuthoredAssetInfo)
        .def("HasAuthoredAssetInfoKey", &UsdObject::HasAuthoredAssetInfoKey,
             arg("keyPath"))
        
        .def("GetDocumentation", &UsdObject::GetDocumentation)
        .def("SetDocumentation", &UsdObject::SetDocumentation, arg("doc"))
        .def("ClearDocumentation", &UsdObject::ClearDocumentation)
        .def("HasAuthoredDocumentation", &UsdObject::HasAuthoredDocumentation)

        .def("GetDisplayName", &UsdObject::GetDisplayName)
        .def("SetDisplayName", &UsdObject::SetDisplayName, arg("name"))
        .def("ClearDisplayName", &UsdObject::ClearDisplayName)
        .def("HasAuthoredDisplayName", &UsdObject::HasAuthoredDisplayName)

        .def("GetNamespaceDelimiter", &UsdObject::GetNamespaceDelimiter)
        .staticmethod("GetNamespaceDelimiter")

        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = object(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);

    TfPyRegisterStlSequencesFromPython<UsdObject>();
}
