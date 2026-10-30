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
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

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
                         nb::handle obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, /*keyPath*/TfToken(), obj, &value) &&
        self.SetMetadata(key, value);
}

static bool _SetMetadataByDictKey(const UsdObject &self, const TfToken& key,
                                  const TfToken &keyPath, nb::handle obj) {
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

static void _SetCustomData(UsdObject &self, nb::handle obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->CustomData, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetCustomData(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetCustomDataByKey(
    UsdObject &self, const TfToken &keyPath, nb::handle obj) {
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

static void _SetAssetInfo(UsdObject &self, nb::handle obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->AssetInfo, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetAssetInfo(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetAssetInfoByKey(
    UsdObject &self, const TfToken &keyPath, nb::handle obj) {
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
// validity. Note that this TfPyObjWrapper is intentionally leaked to avoid
// running Python refcount operations in its d'tor during process shutdown, 
// which is unsafe if Python has been finalized.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdObject.
static nb::object
__getattribute__(nb::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', if the
    // object's prim is valid, or if the attribute is one of a specific
    // inclusion list.
    if ((name[0] == '_' && name[1] == '_') ||
        nb::cast<UsdObject &>(selfObj).GetPrim().IsValid() ||
        strcmp(name, "IsValid") == 0 ||
        strcmp(name, "GetDescription") == 0 ||
        strcmp(name, "GetPrim") == 0 ||
        strcmp(name, "GetPath") == 0 ||
        strcmp(name, "GetPrimPath") == 0 ||
        strcmp(name, "IsPseudoRoot") == 0){
        // Dispatch to object's __getattribute__.
        return nb::borrow<nb::object>(_object__getattribute__->ptr())(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed %s",
                           TfPyObjectRepr(nb::cast<TfPyObjWrapper>(selfObj).Get()).c_str()));
    }
    // Unreachable.
    return nb::object();
}

} // anonymous namespace 

void wrapUsdObject(nb::module_ &m)
{
    nb::class_<UsdObject> clsObj(m, "Object");
    clsObj
        .def(nb::init<>())
        .def(Usd_ObjectSubclass())
        .def("IsValid", &UsdObject::IsValid)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(!nb::self)
        .def("__hash__", __hash__)

        .def("GetStage", &UsdObject::GetStage)
        .def("GetPath", &UsdObject::GetPath)
        .def("GetPrimPath", &UsdObject::GetPrimPath,
             nb::rv_policy::copy)
        .def("GetPrim", &UsdObject::GetPrim)
        .def("GetName", &UsdObject::GetName,
             nb::rv_policy::copy)
        .def("GetDescription", &UsdObject::GetDescription)

        .def("GetMetadata", _GetMetadata, nb::arg("key"))
        .def("SetMetadata", _SetMetadata, nb::arg("key"), nb::arg("value"))

        .def("ClearMetadata", &UsdObject::ClearMetadata, nb::arg("key"))
        .def("HasMetadata", &UsdObject::HasMetadata, nb::arg("key"))
        .def("HasAuthoredMetadata", &UsdObject::HasAuthoredMetadata, nb::arg("key"))

        .def("GetMetadataByDictKey", _GetMetadataByDictKey,
             nb::arg("key"), nb::arg("keyPath"))
        .def("SetMetadataByDictKey", _SetMetadataByDictKey,
             nb::arg("key"), nb::arg("keyPath"), nb::arg("value"))

        .def("ClearMetadataByDictKey", &UsdObject::ClearMetadataByDictKey,
             nb::arg("key"), nb::arg("keyPath"))
        .def("HasMetadataDictKey", &UsdObject::HasMetadataDictKey,
             nb::arg("key"), nb::arg("keyPath"))
        .def("HasAuthoredMetadataDictKey",
             &UsdObject::HasAuthoredMetadataDictKey,
             nb::arg("key"), nb::arg("keyPath"))

        .def("GetAllMetadata", &UsdObject::GetAllMetadata)
        .def("GetAllAuthoredMetadata", &UsdObject::GetAllAuthoredMetadata)

        .def("IsHidden", &UsdObject::IsHidden)
        .def("SetHidden", &UsdObject::SetHidden, nb::arg("hidden"))
        .def("ClearHidden", &UsdObject::ClearHidden)
        .def("HasAuthoredHidden", &UsdObject::HasAuthoredHidden)

        .def("GetCustomData", _GetCustomData)
        .def("GetCustomDataByKey", _GetCustomDataByKey,
             nb::arg("keyPath"))
        .def("SetCustomData", _SetCustomData, nb::arg("customData"))
        .def("SetCustomDataByKey", _SetCustomDataByKey,
             nb::arg("keyPath"), nb::arg("value"))
        .def("ClearCustomData", &UsdObject::ClearCustomData)
        .def("ClearCustomDataByKey", &UsdObject::ClearCustomDataByKey,
             nb::arg("keyPath"))

        .def("HasCustomData", &UsdObject::HasCustomData)
        .def("HasCustomDataKey", &UsdObject::HasCustomDataKey, nb::arg("keyPath"))
        .def("HasAuthoredCustomData", &UsdObject::HasAuthoredCustomData)
        .def("HasAuthoredCustomDataKey", &UsdObject::HasAuthoredCustomDataKey,
             nb::arg("keyPath"))

        .def("GetAssetInfo", _GetAssetInfo)
        .def("GetAssetInfoByKey", _GetAssetInfoByKey,
             nb::arg("keyPath"))
        .def("SetAssetInfo", _SetAssetInfo, nb::arg("assetInfo"))
        .def("SetAssetInfoByKey", _SetAssetInfoByKey,
             nb::arg("keyPath"), nb::arg("value"))
        .def("ClearAssetInfo", &UsdObject::ClearAssetInfo)
        .def("ClearAssetInfoByKey", &UsdObject::ClearAssetInfoByKey,
             nb::arg("keyPath"))

        .def("HasAssetInfo", &UsdObject::HasAssetInfo)
        .def("HasAssetInfoKey", &UsdObject::HasAssetInfoKey, nb::arg("keyPath"))
        .def("HasAuthoredAssetInfo", &UsdObject::HasAuthoredAssetInfo)
        .def("HasAuthoredAssetInfoKey", &UsdObject::HasAuthoredAssetInfoKey,
             nb::arg("keyPath"))
        
        .def("GetDocumentation", &UsdObject::GetDocumentation)
        .def("SetDocumentation", &UsdObject::SetDocumentation, nb::arg("doc"))
        .def("ClearDocumentation", &UsdObject::ClearDocumentation)
        .def("HasAuthoredDocumentation", &UsdObject::HasAuthoredDocumentation)

        .def("GetDisplayName", &UsdObject::GetDisplayName)
        .def("SetDisplayName", &UsdObject::SetDisplayName, nb::arg("name"))
        .def("ClearDisplayName", &UsdObject::ClearDisplayName)
        .def("HasAuthoredDisplayName", &UsdObject::HasAuthoredDisplayName)

        .def_static("GetNamespaceDelimiter", &UsdObject::GetNamespaceDelimiter)

        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ =
        nb::cast<TfPyObjWrapper>(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);

    TfPyRegisterStlSequencesFromPython<UsdObject>();
    Tf_PyRegisterCustomToPythonConverter(
        typeid(UsdObject),
        +[](void const *p) -> PyObject * {
            return Usd_ObjectSubclassConvertToPython(
                static_cast<UsdObject const *>(p));
        });
}
