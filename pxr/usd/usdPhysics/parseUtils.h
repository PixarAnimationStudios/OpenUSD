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
#ifndef USDPHYSICS_PARSE_UTILS_H
#define USDPHYSICS_PARSE_UTILS_H

/// \file usdPhysics/parseUtils.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usdPhysics/parseDesc.h"
#include "pxr/usd/usdPhysics/parsePrimIterator.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //
// PHYSICSPARSEUTILS                                                          //
// -------------------------------------------------------------------------- //


/// UsdPhysicsReportFn - report function that reports parsed data
///
/// \param[in] type     Object type
/// \param[in] numDesc  Number of descriptors to be processed
/// \param[in] primPaths    Array of prim paths that were parsed
/// \param[in] objectDescs  Corresponding array of object descriptors of the reported type
/// \param[in] userData User data provided to the parsing function
using UsdPhysicsReportFn =
    std::function<void(UsdPhysicsObjectType::Enum type, size_t numDesc, const SdfPath* primPaths,
        const UsdPhysicsObjectDesc* objectDescs, void* userData)>;

/// \struct CustomUsdPhysicsTokens
///
/// Token lists for custom physics objects
///
struct CustomUsdPhysicsTokens
{
    std::vector<TfToken> jointTokens;       ///< Custom joints to be reported by parsing
    std::vector<TfToken> shapeTokens;       ///< Custom shapes to be reported by parsing
    std::vector<TfToken> instancerTokens;   ///< Custom physics instancers to be skipped by parsing
};

/// Load USD physics from a given range
///
/// \param[in] stage      Stage to parse
/// \param[in] range      USDRange to parse
/// \param[in] reportFn   Report function that gets parsed USD physics data
/// \param[in] userData   User data passed to report function
/// \param[in] customPhysicsTokens Custom tokens to be reported by the parsing
/// \param[in] simulationOwners List of simulation owners that should be parsed, adding SdfPath() 
///                         indicates that objects without a simulation owner should be parsed too.
/// \return True if load was successful
USDPHYSICS_API bool LoadUsdPhysicsFromRange(const UsdStageWeakPtr stage,
        ParsePrimIteratorBase& range,
        UsdPhysicsReportFn reportFn,
        void* userData,
        const CustomUsdPhysicsTokens* customPhysicsTokens = nullptr,
        const std::vector<SdfPath>* simulationOwners = nullptr);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
