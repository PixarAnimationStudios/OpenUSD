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
#include "pxr/usd/sdf/propertySpec.h"

#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tracelite/trace.h"

#include <ostream>

SDF_DEFINE_ABSTRACT_SPEC(SdfPropertySpec, SdfSpec);

//
// Name
//

const std::string &
SdfPropertySpec::GetName() const
{
    return GetPath().GetName();
}

TfToken
SdfPropertySpec::GetNameToken() const
{
    return GetPath().GetNameToken();
}

bool
SdfPropertySpec::CanSetName(const std::string &newName,
                               std::string *whyNot) const
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::CanRename(
        *this, TfToken(newName)).IsAllowed(whyNot);
}

bool
SdfPropertySpec::SetName(const std::string &newName,
                        bool validate)
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::Rename(
        *this, TfToken(newName));
}

bool
SdfPropertySpec::IsValidName(const std::string &name)
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::IsValidName(name);
}

//
// Ownership
//

SdfSpecHandle
SdfPropertySpec::GetOwner() const
{
    SdfPath parentPath = GetPath().GetParentPath();

    // If this spec is a relational attribute, its parent path will be
    // a target path. Since Sdf does not provide specs for relationship targets
    // we return the target's owning relationship instead.
    if (parentPath.IsTargetPath()) {
        parentPath = parentPath.GetParentPath();
    }
    
    return GetLayer()->GetObjectAtPath(parentPath);
}

//
// Metadata, Property Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfPropertySpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

// Metadata
SDF_DEFINE_GET_SET(DisplayGroup,     SdfFieldKeys->DisplayGroup,     std::string)
SDF_DEFINE_GET_SET(DisplayName,      SdfFieldKeys->DisplayName,      std::string)
SDF_DEFINE_GET_SET(Documentation,    SdfFieldKeys->Documentation,    std::string)
SDF_DEFINE_GET_SET(Hidden,           SdfFieldKeys->Hidden,           bool)
SDF_DEFINE_GET_SET(Prefix,           SdfFieldKeys->Prefix,           std::string)
SDF_DEFINE_GET_SET(SymmetricPeer,    SdfFieldKeys->SymmetricPeer,    std::string)
SDF_DEFINE_GET_SET(SymmetryFunction, SdfFieldKeys->SymmetryFunction, TfToken)

SDF_DEFINE_TYPED_GET_SET(Permission, SdfFieldKeys->Permission, 
                        SdfPermission, SdfPermission)

SDF_DEFINE_DICTIONARY_GET_SET(GetCustomData, SetCustomData,
                             SdfFieldKeys->CustomData);
SDF_DEFINE_DICTIONARY_GET_SET(GetSymmetryArguments, SetSymmetryArgument,
                             SdfFieldKeys->SymmetryArguments);
SDF_DEFINE_DICTIONARY_GET_SET(GetAssetInfo, SetAssetInfo,
                             SdfFieldKeys->AssetInfo);

// Property Value API
// Note: Default value is split up into individual macro calls as the Set
//       requires a boolean return and there's no more-convenient way to
//       shanghai the accessor macros to provide that generically.
SDF_DEFINE_GET(DefaultValue,   SdfFieldKeys->Default, VtValue)
SDF_DEFINE_HAS(DefaultValue,   SdfFieldKeys->Default)
SDF_DEFINE_CLEAR(DefaultValue, SdfFieldKeys->Default)

// Spec Properties
SDF_DEFINE_IS_SET(Custom, SdfFieldKeys->Custom)

SDF_DEFINE_GET_SET(Comment, SdfFieldKeys->Comment, std::string)

SDF_DEFINE_GET(Variability, SdfFieldKeys->Variability, SdfVariability)

// See comment in GetTypeName()
SDF_DEFINE_GET_PRIVATE(AttributeValueTypeName, SdfFieldKeys->TypeName, TfToken)

// Clean up macro shenanigans
#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE

//
// Metadata, Property Value API, and Spec Properties
// (methods requiring additional logic)
//

bool
SdfPropertySpec::SetDefaultValue(const VtValue &defaultValue)
{
    if (defaultValue.IsEmpty()) {
        ClearDefaultValue();
        return true;
    }

    if (defaultValue.IsHolding<SdfValueBlock>()) {
        return SetField(SdfFieldKeys->Default, defaultValue);
    }

    TfType valueType = GetValueType();
    if (valueType.IsUnknown()) {
        TF_CODING_ERROR("Can't set value on attribute <%s> with "
                        "unknown type \"%s\"",
                        GetPath().GetText(),
                        GetTypeName().GetAsToken().GetText());
        return false;
    }

    if (ARCH_UNLIKELY(valueType.GetTypeid() == typeid(void))) {
        // valueType may be provided by a plugin that has not been loaded.
        // In that case, we cannot get the type info, which is required to cast.
        // So we load the plugin in that case.
        if (PlugPluginPtr p = 
                PlugRegistry::GetInstance().GetPluginForType(valueType)) {
            p->Load();
        }
    }

    VtValue value = VtValue::CastToTypeid(defaultValue, valueType.GetTypeid());
    if (value.IsEmpty()) {
        TF_CODING_ERROR("Can't set value on <%s> to %s: "
                        "expected a value of type \"%s\"",
                        GetPath().GetText(),
                        TfStringify(defaultValue).c_str(),
                        valueType.GetTypeName().c_str());
        return false;
    }
    return SetField(SdfFieldKeys->Default, value);
}

SdfTimeSampleMap
SdfPropertySpec::GetTimeSampleMap() const
{
    return GetFieldAs<SdfTimeSampleMap>(SdfFieldKeys->TimeSamples);
}

TfType
SdfPropertySpec::GetValueType() const
{
    // The value type of an attribute is specified by the user when it is
    // constructed, while the value type of a relationship is always SdfPath.
    // Normally, one would use virtual functions to encapsulate this difference;
    // however we don't want to use virtuals as SdfSpec and its subclasses are 
    // intended to be simple value types that are merely wrappers around
    // a layer. So, we have this hacky 'virtual' function.
    switch (GetSpecType()) {
    case SdfSpecTypeAttribute:
        return GetSchema().FindType(_GetAttributeValueTypeName()).GetType();

    case SdfSpecTypeRelationship: {
        static const TfType type = TfType::Find<SdfPath>();
        return type;
    }

    default:
        TF_CODING_ERROR("Unrecognized subclass of SdfPropertySpec on <%s>",
                        GetPath().GetText());
        return TfType();
    }
}

SdfValueTypeName
SdfPropertySpec::GetTypeName() const
{
    // See comment in GetValueType().
    switch (GetSpecType()) {
    case SdfSpecTypeAttribute:
        return GetSchema().FindOrCreateType(_GetAttributeValueTypeName());

    case SdfSpecTypeRelationship:
        return SdfValueTypeName();

    default:
        TF_CODING_ERROR("Unrecognized subclass of SdfPropertySpec on <%s>",
                        GetPath().GetText());
        return SdfValueTypeName();
    }
}

bool
SdfPropertySpec::HasOnlyRequiredFields() const
{
    return GetLayer()->_IsInert(GetPath(), true /*ignoreChildren*/, 
                       true /* requiredFieldOnlyPropertiesAreInert */);
}
