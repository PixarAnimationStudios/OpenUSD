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
