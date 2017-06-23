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
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifndef PXRUSDKATANA_BASEMATERIALHELPERS_H
#define PXRUSDKATANA_BASEMATERIALHELPERS_H

class UsdAttribute;
class UsdPrim;
class UsdRelationship;

// Methods for analyzing base/derived material structure
//
// XXX What we're trying to do here has been described as "partial
//     composition" -- in the sense that we are trying to resolve
//     attributes and relationships in a way that temporarily
//     mutes any contributions from specialized classes, so that
//     we can represent the specializes hierarchy in a way that
//     exercises katana's namespace-style inheritance.
//
//     It seems likely that with more time/experience, we may
//     want to move some of this either into UsdShade API, or
//     directly into Usd in some form.  Consider this a first
//     step to demonstrate that we have the functional pieces
//     of a solution, leaving open the question of ideal API
//     for this sort of thing.

// Check if this attribute resolves from across a direct reference arc.
bool PxrUsdKatana_IsAttrValFromDirectReference(const UsdAttribute &attr);

// Check if this attribute resolves from across a specializes arc.
bool PxrUsdKatana_IsAttrValFromBaseMaterial(const UsdAttribute &attr);

// Check if this prim is defined across a specializes arc.
bool PxrUsdKatana_IsPrimDefFromBaseMaterial(const UsdPrim &prim);

// Check if this relationship has targets provided across a specializes arc.
// (Usd doesn't provide a UsdResolveInfo style API for asking where
// relationship targets are authored, so we do it here ourselves.)
bool PxrUsdKatana_AreRelTargetsFromBaseMaterial(const UsdRelationship &rel);


PXR_NAMESPACE_CLOSE_SCOPE

#endif

