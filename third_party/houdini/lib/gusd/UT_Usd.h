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
#ifndef __GUSD_UT_USD_H__
#define __GUSD_UT_USD_H__


#include "gusd/UT_Error.h"

#include <pxr/pxr.h>
#include "pxr/base/tf/token.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <UT/UT_Array.h>
#include <UT/UT_Error.h>
#include <UT/UT_ErrorManager.h>
#include <UT/UT_StringHolder.h>

#include <string>

class OP_Node;

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<std::pair<std::string, std::string> >
    GusdVariantSelection;
typedef std::vector<std::pair<SdfPath, GusdVariantSelection> >
    GusdVariantSelectionVec;

UsdStageRefPtr GusdUT_GetStage(
        const char* file,
        std::string* err=NULL);

UsdStageRefPtr GusdUT_GetStage(
        const char* file,
        SdfLayerHandle sessionLayer,
        std::string* err=NULL);

UsdStageRefPtr GusdUT_GetStage(
        const char* file,
        const GusdVariantSelectionVec& variantSelections,
        std::string* err=NULL);

bool
GusdUT_GetLayer(
    const char* file,
    SdfLayerRefPtr& layer,
    GusdUT_ErrorContext* err=NULL);


/** Load @a file as a USD stage and fetch the prim at @a primPath from it.
    If @a session is provided, the session will be applied when loading the
    stage. */
UsdPrim GusdUT_GetPrim(
        const char* file,
        const char* primPath,
        std::string* err=NULL);

/** Fetch a prim at the given path in a stage.
    This provides a common error message for lookup failures.*/
UsdPrim GusdUT_GetPrim(const UsdStageRefPtr& stage,
                       const SdfPath& primPath,
                       std::string* err=NULL);

/** Parse and construct an SdfPath from @a pathStr.
    Parse errors are collected in @a errs.
    Returns true if there were no parse errors.
    If @a primPath is empty, true is still returned, and @a path is
    untouched (left empty).*/
bool GusdUT_CreateSdfPath(const char* pathStr,
                          SdfPath& path,
                          GusdUT_ErrorContext* err=NULL);


void GusdUT_GetInheritedPrimInfo(
        const UsdPrim& prim,
        bool& active,
        TfToken& purpose);


PXR_NAMESPACE_CLOSE_SCOPE                        

#endif // __GUSD_UT_USD_H__


