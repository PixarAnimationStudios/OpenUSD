//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file ProxyPolicies.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/proxyPolicies.h"

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE
