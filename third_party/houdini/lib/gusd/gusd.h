//
// Copyright 2017 Pixar
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
#ifndef __GUSD_PXGUSD_H__
#define __GUSD_PXGUSD_H__

#include "pxr/usd/usd/prim.h"
#include "gusd/api.h"

#include "pxr/pxr.h"
#include <string>
#include <functional>

class GA_PrimitiveFactory;

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

GUSD_API
void GusdInit();

GUSD_API
void GusdNewGeometryPrim( GA_PrimitiveFactory* f );

GUSD_API
void GusdNewGeometryIO();

// We sometimes need to be able to convert an absolute path to an asset to a 
// path that can be resolved to an asset using lib Ar. How this is done depends
// on the site specific system used to resolve assets, so we provide a callback.
typedef std::function<std::string (const std::string&)> GusdPathComputeFunc;

GUSD_API
void GusdRegisterComputeRelativeSearchPathFunc( const GusdPathComputeFunc &func );

GUSD_API
std::string GusdComputeRelativeSearchPath( const std::string &path );

// When we write a new asset to a USD file, we want to assign a "kind". By
// default we mark it a "component". 
GUSD_API
void GusdSetAssetKind( const TfToken &kind );

GUSD_API
TfToken GusdGetAssetKind();

// We often need to call some function, which may be system
// specific, on a USD prim so we provide a callback.
typedef std::function<bool (const UsdPrim&)> GusdUsdPrimFunc;

GUSD_API
void GusdRegisterOperateOnUsdPrimFunc( const GusdUsdPrimFunc &func );

GUSD_API
bool GusdOperateOnUsdPrim( const UsdPrim &prim );

PXR_NAMESPACE_CLOSE_SCOPE

#endif
