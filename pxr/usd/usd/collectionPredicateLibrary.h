//
// Copyright 2023 Pixar
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
///     or in the UsdKindRegistry::IsA() sense otherwise).  If the given object
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
SDF_API
UsdObjectPredicateLibrary const &
UsdGetCollectionPredicateLibrary();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_COLLECTION_PREDICATE_LIBRARY_H

