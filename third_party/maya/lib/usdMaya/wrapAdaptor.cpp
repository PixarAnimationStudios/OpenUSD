//
// Copyright 2018 Pixar
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
#include "usdMaya/adaptor.h"
#include "usdMaya/undoHelperCmd.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/pyConversions.h"

#include <maya/MObject.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

static PxrUsdMayaAdaptor*
_Adaptor__init__(const std::string& dagPath)
{
    MObject object;
    MStatus status = PxrUsdMayaUtil::GetMObjectByName(dagPath, object);
    if (!status) {
        return new PxrUsdMayaAdaptor(MObject::kNullObj);
    }

    return new PxrUsdMayaAdaptor(object);
}

static boost::python::object
_Adaptor_GetMetadata(const PxrUsdMayaAdaptor& self, const TfToken& key)
{
    VtValue value;
    if (self.GetMetadata(key, &value)) {
        return boost::python::object(value);
    }
    return boost::python::object();
}

static bool
_Adaptor_SetMetadata(
    PxrUsdMayaAdaptor& self,
    const TfToken& key,
    const VtValue& value)
{
    return PxrUsdMayaUndoHelperCmd::ExecuteWithUndo<bool>(
            [&self, &key, &value](MDGModifier& modifier) {
                return self.SetMetadata(key, value, modifier);
            });
}

static void
_Adaptor_ClearMetadata(
    PxrUsdMayaAdaptor& self,
    const TfToken& key)
{
    PxrUsdMayaUndoHelperCmd::ExecuteWithUndo(
            [&self, &key](MDGModifier& modifier) {
                self.ClearMetadata(key, modifier);
            });
}

static PxrUsdMayaAdaptor::SchemaAdaptor
_Adaptor_ApplySchema(
    PxrUsdMayaAdaptor& self,
    const TfType& ty)
{
    typedef PxrUsdMayaAdaptor::SchemaAdaptor Result;
    return PxrUsdMayaUndoHelperCmd::ExecuteWithUndo<Result>(
            [&self, &ty](MDGModifier& modifier) {
                return self.ApplySchema(ty, modifier);
            });
}

static PxrUsdMayaAdaptor::SchemaAdaptor
_Adaptor_ApplySchemaByName(
    PxrUsdMayaAdaptor& self,
    const TfToken& schemaName)
{
    typedef PxrUsdMayaAdaptor::SchemaAdaptor Result;
    return PxrUsdMayaUndoHelperCmd::ExecuteWithUndo<Result>(
            [&self, &schemaName](MDGModifier& modifier) {
                return self.ApplySchemaByName(schemaName, modifier);
            });
}

static void
_Adaptor_UnapplySchema(
    PxrUsdMayaAdaptor& self,
    const TfType& ty)
{
    PxrUsdMayaUndoHelperCmd::ExecuteWithUndo(
            [&self, &ty](MDGModifier& modifier) {
                self.UnapplySchema(ty, modifier);
            });
}

static void
_Adaptor_UnapplySchemaByName(
    PxrUsdMayaAdaptor& self,
    const TfToken& schemaName)
{
    PxrUsdMayaUndoHelperCmd::ExecuteWithUndo(
            [&self, &schemaName](MDGModifier& modifier) {
                self.UnapplySchemaByName(schemaName, modifier);
            });
}

static std::string
_Adaptor__repr__(const PxrUsdMayaAdaptor& self)
{
    if (self) {
        return TfStringPrintf("%sAdaptor('%s')",
                TF_PY_REPR_PREFIX.c_str(),
                self.GetMayaNodeName().c_str());
    }
    else {
        return "invalid adaptor";
    }
}

static PxrUsdMayaAdaptor::AttributeAdaptor
_SchemaAdaptor_CreateAttribute(
    PxrUsdMayaAdaptor::SchemaAdaptor& self,
    const TfToken& attrName)
{
    typedef PxrUsdMayaAdaptor::AttributeAdaptor Result;
    return PxrUsdMayaUndoHelperCmd::ExecuteWithUndo<Result>(
            [&self, &attrName](MDGModifier& modifier) {
                return self.CreateAttribute(attrName, modifier);
            });
}

static void
_SchemaAdaptor_RemoveAttribute(
    PxrUsdMayaAdaptor::SchemaAdaptor& self,
    const TfToken& attrName)
{
    PxrUsdMayaUndoHelperCmd::ExecuteWithUndo(
            [&self, &attrName](MDGModifier& modifier) {
                return self.RemoveAttribute(attrName, modifier);
            });
}

static std::string
_SchemaAdaptor__repr__(const PxrUsdMayaAdaptor::SchemaAdaptor& self)
{
    if (self) {
        return TfStringPrintf("%s.GetSchemaByName('%s')",
                TfPyRepr(self.GetNodeAdaptor()).c_str(),
                self.GetName().GetText());
    }
    else {
        return "invalid schema adaptor";
    }
}

static boost::python::object
_AttributeAdaptor_Get(const PxrUsdMayaAdaptor::AttributeAdaptor& self)
{
    VtValue value;
    if (self.Get(&value)) {
        return boost::python::object(value);
    }
    return boost::python::object();
}

static bool
_AttributeAdaptor_Set(
    PxrUsdMayaAdaptor::AttributeAdaptor& self,
    const VtValue& value)
{
    return PxrUsdMayaUndoHelperCmd::ExecuteWithUndo<bool>(
            [&self, &value](MDGModifier& modifier) {
                return self.Set(value, modifier);
            });
}

static std::string
_AttributeAdaptor__repr__(const PxrUsdMayaAdaptor::AttributeAdaptor& self)
{
    std::string schemaName;
    const SdfAttributeSpecHandle attrDef = self.GetAttributeDefinition();
    if (TF_VERIFY(attrDef)) {
        const SdfPrimSpecHandle schemaDef =
                TfDynamic_cast<const SdfPrimSpecHandle>(attrDef->GetOwner());
        if (TF_VERIFY(schemaDef)) {
            schemaName = schemaDef->GetName();
        }
    }

    if (self) {
        return TfStringPrintf("%s.GetSchemaByName('%s').GetAttribute('%s')",
                TfPyRepr(self.GetNodeAdaptor()).c_str(),
                schemaName.c_str(),
                self.GetName().GetText());
    }
    else {
        return "invalid attribute adaptor";
    }
}

void wrapAdaptor()
{
    typedef PxrUsdMayaAdaptor This;
    scope Adaptor = class_<This>("Adaptor", no_init)
        .def(!self)
        .def("__init__", make_constructor(_Adaptor__init__))
        .def("__repr__", _Adaptor__repr__)
        .def("GetMayaNodeName", &This::GetMayaNodeName)
        .def("GetUsdTypeName", &This::GetUsdTypeName)
        .def("GetUsdType", &This::GetUsdType)
        .def("GetAppliedSchemas", &This::GetAppliedSchemas)
        .def("GetSchema", &This::GetSchema)
        .def("GetSchemaByName",
                (This::SchemaAdaptor (This::*)(const TfToken&) const)
                &This::GetSchemaByName)
        .def("GetSchemaOrInheritedSchema",
                (This::SchemaAdaptor (This::*)(const TfType&) const)
                &This::GetSchemaOrInheritedSchema)
        .def("ApplySchema", _Adaptor_ApplySchema)
        .def("ApplySchemaByName", _Adaptor_ApplySchemaByName)
        .def("UnapplySchema", _Adaptor_UnapplySchema)
        .def("UnapplySchemaByName", _Adaptor_UnapplySchemaByName)
        .def("GetAllAuthoredMetadata", &This::GetAllAuthoredMetadata,
                return_value_policy<TfPyMapToDictionary>())
        .def("GetMetadata", _Adaptor_GetMetadata)
        .def("SetMetadata", _Adaptor_SetMetadata)
        .def("ClearMetadata", _Adaptor_ClearMetadata)
        .def("GetPrimMetadataFields", &This::GetPrimMetadataFields)
        .staticmethod("GetPrimMetadataFields")
        .def("GetRegisteredAPISchemas", &This::GetRegisteredAPISchemas,
                return_value_policy<TfPySequenceToList>())
        .staticmethod("GetRegisteredAPISchemas")
        .def("GetRegisteredTypedSchemas", &This::GetRegisteredTypedSchemas,
                return_value_policy<TfPySequenceToList>())
        .staticmethod("GetRegisteredTypedSchemas")
        .def("RegisterAttributeAlias", &This::RegisterAttributeAlias)
        .staticmethod("RegisterAttributeAlias")
        .def("GetAttributeAliases", &This::GetAttributeAliases)
        .staticmethod("GetAttributeAliases")
    ;

    class_<This::SchemaAdaptor>("SchemaAdaptor")
        .def(!self)
        .def("__repr__", _SchemaAdaptor__repr__)
        .def("GetNodeAdaptor", &This::SchemaAdaptor::GetNodeAdaptor)
        .def("GetName", &This::SchemaAdaptor::GetName)
        .def("GetAttribute", &This::SchemaAdaptor::GetAttribute)
        .def("CreateAttribute", _SchemaAdaptor_CreateAttribute)
        .def("RemoveAttribute", _SchemaAdaptor_RemoveAttribute)
        .def("GetAuthoredAttributeNames",
                &This::SchemaAdaptor::GetAuthoredAttributeNames)
        .def("GetAttributeNames", &This::SchemaAdaptor::GetAttributeNames)
    ;

    class_<This::AttributeAdaptor>("AttributeAdaptor")
        .def(!self)
        .def("__repr__", _AttributeAdaptor__repr__)
        .def("GetNodeAdaptor", &This::AttributeAdaptor::GetNodeAdaptor)
        .def("GetName", &This::AttributeAdaptor::GetName)
        .def("Get", _AttributeAdaptor_Get)
        .def("Set", _AttributeAdaptor_Set)
        .def("GetAttributeDefinition",
                &This::AttributeAdaptor::GetAttributeDefinition)
    ;
}
