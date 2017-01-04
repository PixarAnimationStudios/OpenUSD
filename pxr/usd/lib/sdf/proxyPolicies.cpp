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
/// \file ProxyPolicies.cpp


#include "pxr/usd/sdf/proxyPolicies.h"
#include "pxr/usd/sdf/mapperSpec.h"

//
// SdfConnectionMapperViewPredicate
//

bool
SdfConnectionMapperViewPredicate::operator()(
    const SdfHandle<SdfMapperSpec>& x) const
{
    return x;
}

//
// SdfRelocatesMapProxyValuePolicy
//

SdfRelocatesMapProxyValuePolicy::Type
SdfRelocatesMapProxyValuePolicy::CanonicalizeType(
    const SdfSpecHandle& spec,
    const Type& x)
{
    if (!TF_VERIFY(spec)) {
        return x;
    }

    SdfPath anchor = spec->GetPath();
    Type result;
    TF_FOR_ALL(i, x) {
        result[i->first.MakeAbsolutePath(anchor)] =
            i->second.MakeAbsolutePath(anchor);
    }
    return result;
}

SdfRelocatesMapProxyValuePolicy::key_type
SdfRelocatesMapProxyValuePolicy::CanonicalizeKey(
    const SdfSpecHandle& spec,
    const key_type& x)
{
    return (TF_VERIFY(spec) ? x.MakeAbsolutePath(spec->GetPath()) : x);
}

SdfRelocatesMapProxyValuePolicy::mapped_type
SdfRelocatesMapProxyValuePolicy::CanonicalizeValue(
    const SdfSpecHandle& spec,
    const mapped_type& x)
{
    return (TF_VERIFY(spec) ? x.MakeAbsolutePath(spec->GetPath()) : x);
}

SdfRelocatesMapProxyValuePolicy::value_type
SdfRelocatesMapProxyValuePolicy::CanonicalizePair(
    const SdfSpecHandle& spec,
    const value_type& x)
{
    if (!TF_VERIFY(spec)) {
        return x;
    }

    SdfPath anchor = spec->GetPath();
    return value_type(x.first.MakeAbsolutePath(anchor),
                      x.second.MakeAbsolutePath(anchor));
}

//
// SdfAttributeViewPredicate
//

SdfAttributeViewPredicate::SdfAttributeViewPredicate() :
    SdfGenericSpecViewPredicate(SdfSpecTypeAttribute)
{
    // Do nothing.
}

//
// SdfRelationshipViewPredicate
//

SdfRelationshipViewPredicate::SdfRelationshipViewPredicate() :
    SdfGenericSpecViewPredicate(SdfSpecTypeRelationship)
{
    // Do nothing.
}
