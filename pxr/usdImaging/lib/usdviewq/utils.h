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
#ifndef USDVIEWQ_UTILS_H
#define USDVIEWQ_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdviewq/api.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class UsdStage;

/// \class UsdviewqUtils
///
/// Performance enhancing utilities for usdview.
///
class UsdviewqUtils {
public:

    struct PrimInfo {
        bool hasCompositionArcs;  
        bool isActive;  
        bool isImageable;
        bool isDefined;
        bool isAbstract;
        bool isInMaster;
        bool isInstance;
        bool isVisibilityInherited;
        bool visVaries;
        std::string name;
        std::string typeName;
    };

    /// For the given \p stage and \p schemaType, return all active, defined
    /// prims that either match the schemaType exactly or are a descendant type.
    //
    //      Furthermore, this method is not intended to be used publically,
    //      ultimately Usd will have better core support for fast prim
    //      filtering by typeName. 
    USDVIEWQ_API
    static
    std::vector<UsdPrim> _GetAllPrimsOfType(UsdStagePtr const &stage, 
                                            TfType const& schemaType);

    /// Fetch prim-related data in batch to to speed up Qt treeview item
    /// population.  Takes a time argument so that we can evaluate the prim's
    /// visibiity if it is imageable.
    USDVIEWQ_API
    static UsdviewqUtils::PrimInfo GetPrimInfo(UsdPrim prim, UsdTimeCode time);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDVIEWQ_UTILS_H
