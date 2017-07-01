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
/// \file AttributeSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/markerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tracelite/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(SdfAttributeSpec, SdfPropertySpec);

SdfAttributeSpecHandle
SdfAttributeSpec::New(
    const SdfPrimSpecHandle& owner,
    const std::string& name,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    TRACE_FUNCTION();

    if (!owner) {
	TF_CODING_ERROR("Cannot create an SdfAttributeSpec with a null owner");
	return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR(
            "Cannot create attribute on %s with invalid name: %s",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    SdfPath attributePath = owner->GetPath().AppendProperty(TfToken(name));
    if (!attributePath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Cannot create attribute at invalid path <%s.%s>",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    return _New(owner, attributePath, typeName, variability, custom);
}

SdfAttributeSpecHandle
SdfAttributeSpec::New(
    const SdfRelationshipSpecHandle& owner,
    const SdfPath& targetPath,
    const std::string& name,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner relationship");
        return TfNullPtr;
    }
    return _New(owner, targetPath, name, typeName, variability, custom);
}

SdfAttributeSpecHandle
SdfAttributeSpec::_New(
    const SdfSpecHandle &owner,
    const SdfPath& attrPath,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    if (!owner) {
        TF_CODING_ERROR("NULL owner");
        return TfNullPtr;
    }
    if (!typeName) {
        TF_CODING_ERROR("Cannot create attribute spec <%s> with invalid type",
                        attrPath.GetText());
        return TfNullPtr;
    }

    SdfChangeBlock block;

    // AttributeSpecs are considered initially to have only required fields 
    // only if they are not custom.
    bool hasOnlyRequiredFields = (!custom);

    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::CreateSpec(
            owner->GetLayer(), attrPath, SdfSpecTypeAttribute, 
            hasOnlyRequiredFields)) {
        return TfNullPtr;
    }

    SdfAttributeSpecHandle spec =
	owner->GetLayer()->GetAttributeAtPath(attrPath);

    // Avoid expensive dormancy checks in the case of binary-backed data.
    SdfAttributeSpec *specPtr = get_pointer(spec);
    if (TF_VERIFY(specPtr)) {
        specPtr->SetField(SdfFieldKeys->Custom, custom);
        specPtr->SetField(SdfFieldKeys->TypeName, typeName.GetAsToken());
        specPtr->SetField(SdfFieldKeys->Variability, variability);
    }

    return spec;
}

SdfAttributeSpecHandle
SdfAttributeSpec::_New(
    const SdfRelationshipSpecHandle& owner,
    const SdfPath& path,
    const std::string& name,
    const SdfValueTypeName& typeName,
    SdfVariability variability,
    bool custom)
{
    if (!owner) {
        TF_CODING_ERROR("NULL owner");
        return TfNullPtr;
    }
    if (!typeName) {
        TF_CODING_ERROR("Cannot create attribute spec <%s> with invalid type",
                        owner->GetPath().AppendTarget(path).
                            AppendProperty(TfToken(name)).GetText());
	return TfNullPtr;
    }

    SdfChangeBlock block;

    // Determine the path of the relationship target
    SdfPath absPath = path.MakeAbsolutePath(owner->GetPath().GetPrimPath());
    SdfPath targetPath = owner->GetPath().AppendTarget(absPath);

    // Check to make sure that the name is valid
    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR(
            "Cannot create attribute on %s with invalid name: %s",
            targetPath.GetText(), name.c_str());
        return TfNullPtr;
    }

    // Create the relationship target if it doesn't already exist. Note
    // that this does not automatically get added to the relationship's
    // target path list.
    SdfSpecHandle targetSpec = owner->_FindOrCreateTargetSpec(path);

    // AttributeSpecs are considered initially to have only required fields 
    // only if they are not custom.
    bool hasOnlyRequiredFields = (!custom);

    // Create the relational attribute spec
    SdfPath attrPath = targetPath.AppendRelationalAttribute(TfToken(name));
    if (!Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::CreateSpec(
            owner->GetLayer(), attrPath, SdfSpecTypeAttribute, 
            hasOnlyRequiredFields)) {
        return TfNullPtr;
    }

    SdfAttributeSpecHandle spec =
        owner->GetLayer()->GetAttributeAtPath(attrPath);
    
    // Avoid expensive dormancy checks in the case of binary-backed data.
    SdfAttributeSpec *specPtr = get_pointer(spec);
    if (TF_VERIFY(specPtr)) {
        specPtr->SetField(SdfFieldKeys->Custom, custom);
        specPtr->SetField(SdfFieldKeys->TypeName, typeName.GetAsToken());
        specPtr->SetField(SdfFieldKeys->Variability, variability);
    }

    return spec;
}

//
// Connections
//

SdfPath
SdfAttributeSpec::_CanonicalizeConnectionPath(
    const SdfPath& connectionPath) const
{
    // Attribute connection paths are always absolute. If a relative path
    // is passed in, it is considered to be relative to the connection's
    // owning prim.
    return connectionPath.MakeAbsolutePath(GetPath().GetPrimPath());
}

SdfConnectionsProxy
SdfAttributeSpec::GetConnectionPathList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->ConnectionPaths);
}

bool
SdfAttributeSpec::HasConnectionPaths() const
{
    return GetConnectionPathList().HasKeys();
}

void
SdfAttributeSpec::ClearConnectionPaths()
{
    GetConnectionPathList().ClearEdits();
}

//
// Mappers
//

SdfConnectionMappersProxy
SdfAttributeSpec::GetConnectionMappers() const
{
    return SdfConnectionMappersProxy(SdfConnectionMappersView(
            GetLayer(), GetPath(), SdfChildrenKeys->MapperChildren,
            SdfPathKeyPolicy(SdfCreateHandle(this))),
        "connection mappers",
        SdfConnectionMappersProxy::CanErase);
}

SdfPath
SdfAttributeSpec::GetConnectionPathForMapper(
    const SdfMapperSpecHandle& mapper)
{
    if (mapper->GetAttribute() == SdfCreateHandle(this)) {
        return mapper->GetConnectionTargetPath();
    }
    return SdfPath();
}

void
SdfAttributeSpec::ChangeMapperPath(
    const SdfPath& oldPath, const SdfPath& newPath)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Change mapper path: Permission denied.");
        return;
    }

    const SdfPath& attrPath = GetPath();

    // Absolutize.
    SdfPath oldAbsPath = oldPath.MakeAbsolutePath(attrPath.GetPrimPath());
    SdfPath newAbsPath = newPath.MakeAbsolutePath(attrPath.GetPrimPath());

    // Validate.
    if (oldAbsPath == newAbsPath) {
        // Nothing to do.
        return;
    }
    if (!newAbsPath.IsPropertyPath()) {
        TF_CODING_ERROR("cannot change connection path for attribute %s's "
                        "mapper at connection path <%s> to <%s> because it's "
                        "not a property path",
                        attrPath.GetString().c_str(),
                        oldAbsPath.GetString().c_str(),
                        newAbsPath.GetString().c_str());
        return;
    }
    
    SdfPathVector mapperPaths = 
        GetFieldAs<SdfPathVector>(SdfChildrenKeys->MapperChildren);

    // Check that a mapper actually exists at the old path.
    SdfPathVector::iterator mapperIt = 
        std::find(mapperPaths.begin(), mapperPaths.end(), oldAbsPath);
    if (mapperIt == mapperPaths.end()) {
        TF_CODING_ERROR("Change mapper path: No mapper exists for "
            "connection path <%s>.", oldAbsPath.GetText());
        return;
    }

    // Check that no mapper already exists at the new path.
    const bool mapperExistsAtNewPath = 
        (std::find(mapperPaths.begin(), mapperPaths.end(), newAbsPath) != 
            mapperPaths.end());
    if (mapperExistsAtNewPath) {
        TF_CODING_ERROR("Change mapper path: Mapper already exists for "
            "connection path <%s>.", newAbsPath.GetText());
        return;
    }

    // Things look OK -- let's go ahead and move the mapper over to the
    // new path.
    SdfChangeBlock block;
        
    const SdfPath oldMapperSpecPath = attrPath.AppendMapper(oldAbsPath);
    const SdfPath newMapperSpecPath = attrPath.AppendMapper(newAbsPath);
    _MoveSpec(oldMapperSpecPath, newMapperSpecPath);

    *mapperIt = newAbsPath;
    SetField(SdfChildrenKeys->MapperChildren, VtValue(mapperPaths));

}

//
// Markers
//

SdfSpecHandle 
SdfAttributeSpec::_FindOrCreateChildSpecForMarker(const SdfPath& key)
{
    SdfChangeBlock block;

    const SdfPath targetPath = _CanonicalizeConnectionPath(key);
    const SdfPath connectionSpecPath = GetPath().AppendTarget(targetPath);

    SdfSpecHandle child = GetLayer()->GetObjectAtPath(connectionSpecPath);
    if (!child) {
        Sdf_ChildrenUtils<Sdf_AttributeConnectionChildPolicy>::CreateSpec(
            GetLayer(), connectionSpecPath, SdfSpecTypeConnection);
        child = GetLayer()->GetObjectAtPath(connectionSpecPath);
    }

    if (child) {
        // Insert key into list editor if it's not there.  We must
        // add it because the menva syntax does not support expressing
        // a marker without expressing existence of the corresponding
        // connection path.
        GetConnectionPathList().Add(targetPath);
    }

    return child;
}

std::string 
SdfAttributeSpec::GetConnectionMarker(const SdfPath& path) const
{
    const SdfPath connectionPath = _CanonicalizeConnectionPath(path);
    return Sdf_MarkerUtils<SdfAttributeSpec>::GetMarker(
        static_cast<const SdfAttributeSpec&>(*this), connectionPath);
}

void 
SdfAttributeSpec::SetConnectionMarker(
    const SdfPath& path, const std::string& marker)
{
    const SdfPath connectionPath = _CanonicalizeConnectionPath(path);
    Sdf_MarkerUtils<SdfAttributeSpec>::SetMarker(
        static_cast<SdfAttributeSpec*>(this), connectionPath, marker);
}

void 
SdfAttributeSpec::ClearConnectionMarker(const SdfPath& path)
{
    const SdfPath connectionPath = _CanonicalizeConnectionPath(path);
    Sdf_MarkerUtils<SdfAttributeSpec>::ClearMarker(
        static_cast<SdfAttributeSpec*>(this), connectionPath);
}

SdfPathVector 
SdfAttributeSpec::GetConnectionMarkerPaths() const
{
    return Sdf_MarkerUtils<SdfAttributeSpec>::GetMarkerPaths(
        static_cast<const SdfAttributeSpec&>(*this));
}

void
SdfAttributeSpec::SetConnectionMarkers(const ConnectionMarkerMap& markers)
{
    // Canonicalize all paths in the map before passing along to marker utils.
    Sdf_MarkerUtils<SdfAttributeSpec>::MarkerMap m;
    TF_FOR_ALL(it, markers) {
        m[_CanonicalizeConnectionPath(it->first)] = it->second;
    }

    Sdf_MarkerUtils<SdfAttributeSpec>::SetMarkers(
        static_cast<SdfAttributeSpec*>(this), m);
}

//
// Metadata, Attribute Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfAttributeSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

// Attribute Value API

SDF_DEFINE_GET_SET_HAS_CLEAR(AllowedTokens, SdfFieldKeys->AllowedTokens, VtTokenArray)

SDF_DEFINE_GET_SET_HAS_CLEAR(ColorSpace, SdfFieldKeys->ColorSpace, TfToken)

TfEnum
SdfAttributeSpec::GetDisplayUnit() const
{
    // The difference between this and the macro version is that the
    // macro calls _GetValueWithDefault().  That checks if the value
    // is empty and, if so, returns the default value from the schema.
    // But we want to return a default displayUnit that's based on
    // the role.
    TfEnum displayUnit;
    if (HasField(SdfFieldKeys->DisplayUnit, &displayUnit)) {
        return displayUnit;
    }

    return GetTypeName().GetDefaultUnit();
}

TfToken
SdfAttributeSpec::GetRoleName() const
{
    return GetTypeName().GetRole();
}

SDF_DEFINE_SET(DisplayUnit, SdfFieldKeys->DisplayUnit, const TfEnum&)
SDF_DEFINE_HAS(DisplayUnit, SdfFieldKeys->DisplayUnit)
SDF_DEFINE_CLEAR(DisplayUnit, SdfFieldKeys->DisplayUnit)

PXR_NAMESPACE_CLOSE_SCOPE

// Clean up macro shenanigans
#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE
