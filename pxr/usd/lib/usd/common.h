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

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layerOffset.h"

#include "pxr/usd/usd/primDataHandle.h"
#include "pxr/usd/usd/timeCode.h"

#include <string>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

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
USD_API
std::string UsdDescribe(const UsdObject &);
/// \overload
USD_API
std::string UsdDescribe(const UsdStageRefPtr &);
/// \overload
USD_API
std::string UsdDescribe(const UsdStageWeakPtr &);
/// \overload
USD_API
std::string UsdDescribe(const UsdStage *);
/// \overload
USD_API
std::string UsdDescribe(const UsdStage &);
/// \overload
USD_API
std::string UsdDescribe(const UsdStageCache &);

// XXX:
// Currently used for querying composed values from ascii layers, so VtValue is
// the optimal value-store, but this may not always be the case.
typedef std::map<class TfToken, VtValue,
                 TfDictionaryLessThan
                 > UsdMetadataValueMap;

/// Returns true if the pipeline is configured to process / generate 
/// USD only and stop generating tidScenes.
USD_API
bool UsdIsRetireLumosEnabled();

/// Returns true if Add() methods in the USD API, when given
/// UsdListPositionTempDefault, should author "add" operations
/// in SdfListOp values instead of prepends. Used for backwards
/// compatibility.
USD_API
bool UsdAuthorOldStyleAdd();

/// Returns true if USD uses the historical behavior of applying
/// the inverse of composed layer offsets to map layer time to
/// stage time.  Respects the env setting USD_USE_INVERSE_LAYER_OFFSET.
USD_API
bool UsdUsesInverseLayerOffset();

/// Prepare the given offset for application to map layer time to
/// stage time, respecting the environment variable
/// USD_USE_INVERSE_LAYER_OFFSET.
///
/// Typically, the supplied SdfLayerOffset will come from Pcp -- in
/// a PcpNodeRef or PcpLayerStack -- and represent the cumulative offset
/// to transform data from a layer to the Usd stage.
///
/// Historically, USD applied the inverse of that offset, flipping
/// the intended semantics. To address this, this function provides a
/// temporary measure to control whether to take the inverse or not.
/// Under the new behavior this function will become a no-op,
/// and can eventually be phased out.
USD_API
SdfLayerOffset UsdPrepLayerOffset(SdfLayerOffset offset);

/// \enum UsdListPosition
///
/// Specifies a position to add items to lists.  Used by some Add()
/// methods in the USD API that manipulate lists, such as AddReference().
///
enum UsdListPosition {
    /// The front of the list
    UsdListPositionFront,
    /// The back of the list
    UsdListPositionBack,
    /// Default position.
    /// XXX This value will be removed in the near future. This is
    /// meant as a temporary value used for staged rollout of the
    /// new behavior with a TfEnvSetting.
    UsdListPositionTempDefault,
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
