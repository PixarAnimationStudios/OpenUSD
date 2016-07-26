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
/// \file MapperSpec.cpp
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tracelite/trace.h"

#define SDF_ACCESSOR_CLASS                   SdfMapperSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

SDF_DEFINE_SPEC(SdfMapperSpec, SdfSpec);

SdfMapperSpecHandle
SdfMapperSpec::New(
    const SdfAttributeSpecHandle& owner,
    const SdfPath& connPath,
    const std::string& typeName)
{
    TRACE_FUNCTION();

    if (not owner) {
        TF_CODING_ERROR("NULL owner attribute");
        return TfNullPtr;
    }
    SdfPath absPath = connPath.MakeAbsolutePath(owner->GetPath().GetPrimPath());
    if (not absPath.IsPropertyPath()) {
        TF_CODING_ERROR("A mapper must have a connection path that "
                        "identifies a property.");
        return TfNullPtr;
    }

    SdfPath mapperPath = 
        Sdf_MapperChildPolicy::GetChildPath(owner->GetPath(), connPath);

    SdfChangeBlock block;

    if (not Sdf_ChildrenUtils<Sdf_MapperChildPolicy>::CreateSpec(
            owner->GetLayer(), mapperPath, SdfSpecTypeMapper)) {
	return TfNullPtr;
    }
    
    SdfMapperSpecHandle mapper = TfStatic_cast<SdfMapperSpecHandle>(
        owner->GetLayer()->GetObjectAtPath(mapperPath));
    
    mapper->SetField(SdfFieldKeys->TypeName, typeName);
        
    return mapper;
}

//
// Namespace hierarchy
//

SdfAttributeSpecHandle
SdfMapperSpec::GetAttribute() const
{
    return GetLayer()->GetAttributeAtPath(GetPath().GetParentPath());
}

SdfPath
SdfMapperSpec::GetConnectionTargetPath() const
{
    return GetPath().GetTargetPath();
}

//
// Type
//

SDF_DEFINE_GET_SET(TypeName, SdfFieldKeys->TypeName, std::string)

//
// Args
//

SdfMapperArgsProxy
SdfMapperSpec::GetArgs() const
{
    return SdfMapperArgsProxy(SdfMapperArgSpecView(
        GetLayer(), GetPath(), 
        SdfChildrenKeys->MapperArgChildren),
        "mapper args",
        SdfMapperArgsProxy::CanErase);
}

//
// Symmetry args
//

SDF_DEFINE_DICTIONARY_GET(GetSymmetryArgs, SdfFieldKeys->SymmetryArgs)

// Need specialized implementation of SetSymmetryArgs to set dictionary via
// the edit proxy to ensure necessary validation is performed.
void
SdfMapperSpec::SetSymmetryArgs(const VtDictionary& dict)
{
    GetSymmetryArgs() = dict;
}

#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE
