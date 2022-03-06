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
#include "pxr/usd/usd/object.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/ostreamMethods.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/extract.hpp>

#include <string>
#include <vector>



PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdWrapObject {

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
                         boost::python::object obj) {
    VtValue value;
    return UsdPythonToMetadataValue(key, /*keyPath*/TfToken(), obj, &value) &&
        self.SetMetadata(key, value);
}

static bool _SetMetadataByDictKey(const UsdObject &self, const TfToken& key,
                                  const TfToken &keyPath, boost::python::object obj) {
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

static void _SetCustomData(UsdObject &self, boost::python::object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->CustomData, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetCustomData(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetCustomDataByKey(
    UsdObject &self, const TfToken &keyPath, boost::python::object obj) {
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

static void _SetAssetInfo(UsdObject &self, boost::python::object obj) {
    VtValue value;
    if (UsdPythonToMetadataValue(
            SdfFieldKeys->AssetInfo, TfToken(), obj, &value) &&
        value.IsHolding<VtDictionary>()) {
        self.SetAssetInfo(value.UncheckedGet<VtDictionary>());
    }
}

static void _SetAssetInfoByKey(
    UsdObject &self, const TfToken &keyPath, boost::python::object obj) {
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
static boost::python::object
__getattribute__(boost::python::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', if the
    // object's prim is valid, or if the attribute is one of a specific
    // inclusion list.
    if ((name[0] == '_' && name[1] == '_') ||
        boost::python::extract<UsdObject &>(selfObj)().GetPrim().IsValid() ||
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
    return boost::python::object();
}

} // anonymous namespace 

void wrapUsdObject()
{
    boost::python::class_<UsdObject> clsObj("Object");
    clsObj
        .def(Usd_ObjectSubclass())
        .def("IsValid", &UsdObject::IsValid)

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(!boost::python::self)
        .def("__hash__", pxrUsdUsdWrapObject::__hash__)

        .def("GetStage", &UsdObject::GetStage)
        .def("GetPath", &UsdObject::GetPath)
        .def("GetPrimPath", &UsdObject::GetPrimPath,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetPrim", &UsdObject::GetPrim)
        .def("GetName", &UsdObject::GetName,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetDescription", &UsdObject::GetDescription)

        .def("GetMetadata", pxrUsdUsdWrapObject::_GetMetadata, boost::python::arg("key"))
        .def("SetMetadata", pxrUsdUsdWrapObject::_SetMetadata, (boost::python::arg("key"), boost::python::arg("value")))

        .def("ClearMetadata", &UsdObject::ClearMetadata, boost::python::arg("key"))
        .def("HasMetadata", &UsdObject::HasMetadata, boost::python::arg("key"))
        .def("HasAuthoredMetadata", &UsdObject::HasAuthoredMetadata, boost::python::arg("key"))

        .def("GetMetadataByDictKey", pxrUsdUsdWrapObject::_GetMetadataByDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath")))
        .def("SetMetadataByDictKey", pxrUsdUsdWrapObject::_SetMetadataByDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath"), boost::python::arg("value")))

        .def("ClearMetadataByDictKey", &UsdObject::ClearMetadataByDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath")))
        .def("HasMetadataDictKey", &UsdObject::HasMetadataDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath")))
        .def("HasAuthoredMetadataDictKey",
             &UsdObject::HasAuthoredMetadataDictKey,
             (boost::python::arg("key"), boost::python::arg("keyPath")))

        .def("GetAllMetadata", &UsdObject::GetAllMetadata,
             boost::python::return_value_policy<TfPyMapToDictionary>())
        .def("GetAllAuthoredMetadata", &UsdObject::GetAllAuthoredMetadata,
             boost::python::return_value_policy<TfPyMapToDictionary>())

        .def("IsHidden", &UsdObject::IsHidden)
        .def("SetHidden", &UsdObject::SetHidden, boost::python::arg("hidden"))
        .def("ClearHidden", &UsdObject::ClearHidden)
        .def("HasAuthoredHidden", &UsdObject::HasAuthoredHidden)

        .def("GetCustomData", pxrUsdUsdWrapObject::_GetCustomData)
        .def("GetCustomDataByKey", pxrUsdUsdWrapObject::_GetCustomDataByKey,
             boost::python::arg("keyPath"))
        .def("SetCustomData", pxrUsdUsdWrapObject::_SetCustomData, boost::python::arg("customData"))
        .def("SetCustomDataByKey", pxrUsdUsdWrapObject::_SetCustomDataByKey,
             (boost::python::arg("keyPath"), boost::python::arg("value")))
        .def("ClearCustomData", &UsdObject::ClearCustomData)
        .def("ClearCustomDataByKey", &UsdObject::ClearCustomDataByKey,
             boost::python::arg("keyPath"))

        .def("HasCustomData", &UsdObject::HasCustomData)
        .def("HasCustomDataKey", &UsdObject::HasCustomDataKey, boost::python::arg("keyPath"))
        .def("HasAuthoredCustomData", &UsdObject::HasAuthoredCustomData)
        .def("HasAuthoredCustomDataKey", &UsdObject::HasAuthoredCustomDataKey,
             boost::python::arg("keyPath"))

        .def("GetAssetInfo", pxrUsdUsdWrapObject::_GetAssetInfo)
        .def("GetAssetInfoByKey", pxrUsdUsdWrapObject::_GetAssetInfoByKey,
             boost::python::arg("keyPath"))
        .def("SetAssetInfo", pxrUsdUsdWrapObject::_SetAssetInfo, boost::python::arg("assetInfo"))
        .def("SetAssetInfoByKey", pxrUsdUsdWrapObject::_SetAssetInfoByKey,
             (boost::python::arg("keyPath"), boost::python::arg("value")))
        .def("ClearAssetInfo", &UsdObject::ClearAssetInfo)
        .def("ClearAssetInfoByKey", &UsdObject::ClearAssetInfoByKey,
             boost::python::arg("keyPath"))

        .def("HasAssetInfo", &UsdObject::HasAssetInfo)
        .def("HasAssetInfoKey", &UsdObject::HasAssetInfoKey, boost::python::arg("keyPath"))
        .def("HasAuthoredAssetInfo", &UsdObject::HasAuthoredAssetInfo)
        .def("HasAuthoredAssetInfoKey", &UsdObject::HasAuthoredAssetInfoKey,
             boost::python::arg("keyPath"))
        
        .def("GetDocumentation", &UsdObject::GetDocumentation)
        .def("SetDocumentation", &UsdObject::SetDocumentation, boost::python::arg("doc"))
        .def("ClearDocumentation", &UsdObject::ClearDocumentation)
        .def("HasAuthoredDocumentation", &UsdObject::HasAuthoredDocumentation)

        .def("GetNamespaceDelimiter", &UsdObject::GetNamespaceDelimiter)
        .staticmethod("GetNamespaceDelimiter")

        ;

    // Save existing __getattribute__ and replace.
    *pxrUsdUsdWrapObject::_object__getattribute__ = boost::python::object(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", pxrUsdUsdWrapObject::__getattribute__);

    TfPyRegisterStlSequencesFromPython<UsdObject>();
}
