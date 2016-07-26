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
/// \fild MapperArgSpec.cpp
#include "pxr/usd/sdf/mapperArgSpec.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/tracelite/trace.h"

#define SDF_ACCESSOR_CLASS                   SdfMapperArgSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

SDF_DEFINE_SPEC(SdfMapperArgSpec, SdfSpec);

SdfMapperArgSpecHandle
SdfMapperArgSpec::New(
    const SdfMapperSpecHandle& owner, 
    const std::string& name, 
    const VtValue& value)
{
    TRACE_FUNCTION();

    if (not owner) {
        TF_CODING_ERROR("NULL owner mapper");
        return TfNullPtr;
    }

    if (not Sdf_ChildrenUtils<Sdf_MapperArgChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR("Cannot create mapper arg on %s with "
            "invalid name: '%s'", owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    const SdfSchema::FieldDefinition* mapperArgValueDef = 
        owner->GetSchema().GetFieldDefinition(SdfFieldKeys->MapperArgValue);
    if (SdfAllowed validArgValue = mapperArgValueDef->IsValidValue(value)) {
        // Do nothing
    }
    else {
        TF_CODING_ERROR("Cannot create mapper arg '%s' on <%s>: %s",
            name.c_str(), owner->GetPath().GetText(), 
            validArgValue.GetWhyNot().c_str());
        return TfNullPtr;
    }

    SdfPath argPath = owner->GetPath().AppendMapperArg(TfToken(name));

    SdfChangeBlock block;

    if (not Sdf_ChildrenUtils<Sdf_MapperArgChildPolicy>::CreateSpec(
	    owner->GetLayer(), argPath, SdfSpecTypeMapperArg)) {
        return TfNullPtr;
    }

    SdfMapperArgSpecHandle arg = TfStatic_cast<SdfMapperArgSpecHandle>(
        owner->GetLayer()->GetObjectAtPath(argPath));
    
    arg->SetField(SdfFieldKeys->MapperArgValue, value);
    
    return arg;
}

//
// Namespace hierarchy
//

SdfMapperSpecHandle
SdfMapperArgSpec::GetMapper() const
{
    return TfDynamic_cast<SdfMapperSpecHandle>(
        GetLayer()->GetObjectAtPath(GetPath().GetParentPath()));
}

const std::string&
SdfMapperArgSpec::GetName() const
{
    return GetPath().GetName();
}

const TfToken&
SdfMapperArgSpec::GetNameToken() const
{
    return GetPath().GetNameToken();
}

void
SdfMapperArgSpec::SetName(const std::string& name)
{
    Sdf_ChildrenUtils<Sdf_MapperArgChildPolicy>::Rename(
        *this, TfToken(name));
}

//
// Value
//

SDF_DEFINE_GET_SET(Value, SdfFieldKeys->MapperArgValue, VtValue)

#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE
