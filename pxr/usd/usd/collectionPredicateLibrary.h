//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_COLLECTION_PREDICATE_LIBRARY_H
#define PXR_USD_USD_COLLECTION_PREDICATE_LIBRARY_H

/// \file usd/collectionPredicateLibrary.h

#include "pxr/pxr.h"

#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/predicateLibrary.h"

PXR_NAMESPACE_OPEN_SCOPE

using UsdObjectPredicateLibrary = SdfPredicateLibrary<UsdObject>;

/// Return the predicate library used to evaluate SdfPathExpressions in
/// UsdCollectionAPI's membershipExpression attributes.
///
/// This library provides the following predicate functions.  Where the
/// documentation says closest-prim, it means the given object if that object is
/// a prim, otherwise the object's owning prim if the object is a property.
///
/// abstract(bool isAbstract=true)
///     Return true if the closest-prim's IsAbstract() == isAbstract.
///
/// defined(bool isDefined=true)
///     Return true if the closest-prim's IsDefined() == isDefined.
///
/// model(bool isModel=true)
///     Return true if the given object is a prim and its IsModel() == isModel.
///     If the given object is not a prim, return false.
///
/// group(bool isGroup=true)
///     Return true if the given object is a prim and its IsGroup() == isGroup.
///     If the given object is not a prim, return false.
///
/// kind(kind1, ... kindN, strict=false)
///     Return true if the given object is a prim, and its kind metadata (see
///     UsdModelAPI::GetKind()) is one of kind1...kindN (exactly if strict=true,
///     or in the KindRegistry::IsA() sense otherwise).  If the given object
///     is not a prim, return false.
///
/// specifier(spec1, ... specN)
///     Return true if the given object is a prim and its specifier (see
///     UsdPrim::GetSpecifier()) is one of spec1...specN.  The spec1..specN
///     arguments must be unnamed strings: "over", "class", or "def".  If the
///     given object is not a prim, return false.
///
/// isa(schema1, ... schemaN, strict=false)
///     Return true if the given object is a prim and its typed schema (see
///     UsdPrim::IsA())is exactly one of schema1...schemaN if strict=true, or a
///     subtype of schema1...schemaN if strict=false.  If the given object is
///     not a prim, return false.
///
/// hasAPI(apiSchema1, ... apiSchemaN, [instanceName=name])
///     Return true if the given object is a prim and it has an applied API
///     schema (see UsdPrim::HasAPI()) of type apiSchema1...apiSchemaN.  If the
///     'instanceName' argument is supplied, the prim must have an applied API
///     schema with that instanceName.  If the given object is not a prim,
///     return false.
///
/// variant(set1=selGlob1, ... setN=selGlobN)
///     Return true if the given object is a prim and it has selections matching
///     the literal names or glob patterns selGlob1...selGlobN for the variant
///     sets set1...setN.  See UsdPrim::GetVariantSets() and
///     UsdVariantSets::GetVariantSelection().  If the given object is not a
///     prim, return false.
///
USD_API
UsdObjectPredicateLibrary const &
UsdGetCollectionPredicateLibrary();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_COLLECTION_PREDICATE_LIBRARY_H

