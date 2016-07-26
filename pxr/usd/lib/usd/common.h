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
#ifndef USD_COMMON_H
#define USD_COMMON_H

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usd/primDataHandle.h"
#include "pxr/usd/usd/timeCode.h"

#include <string>
#include <map>

// Forward declare Usd classes.
class UsdStage;
class UsdObject;
class UsdPrim;
class UsdProperty;
class UsdAttribute;
class UsdRelationship;
class UsdStageCache;

class VtValue;

TF_DECLARE_WEAK_AND_REF_PTRS(UsdStage);
typedef UsdStagePtr UsdStageWeakPtr;

/// Return a human-readable description.
std::string UsdDescribe(const UsdObject &);
/// \overload
std::string UsdDescribe(const UsdStageRefPtr &);
/// \overload
std::string UsdDescribe(const UsdStageWeakPtr &);
/// \overload
std::string UsdDescribe(const UsdStage *);
/// \overload
std::string UsdDescribe(const UsdStage &);
/// \overload
std::string UsdDescribe(const UsdStageCache &);

// XXX:
// Currently used for querying composed values from ascii layers, so VtValue is
// the optimal value-store, but this may not always be the case.
typedef std::map<class TfToken, VtValue,
                 TfDictionaryLessThan
                 > UsdMetadataValueMap;

/// Returns true if the pipeline is configured to process / generate 
/// USD only and stop generating tidScenes.
bool UsdIsRetireLumosEnabled();

#endif
