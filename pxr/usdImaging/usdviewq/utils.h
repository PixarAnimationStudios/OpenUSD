//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USDVIEWQ_UTILS_H
#define PXR_USD_IMAGING_USDVIEWQ_UTILS_H

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
        PrimInfo(const UsdPrim &prim, const UsdTimeCode time);

        bool hasCompositionArcs;  
        bool isActive;  
        bool isImageable;
        bool isDefined;
        bool isAbstract;
        bool isInPrototype;
        bool isInstance;
        bool supportsGuides;
        bool supportsDrawMode;
        bool isVisibilityInherited;
        bool visVaries;
        std::string name;
        std::string typeName;
        std::string displayName;
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
    static UsdviewqUtils::PrimInfo GetPrimInfo(const UsdPrim &prim, 
                                               const UsdTimeCode time);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USDVIEWQ_UTILS_H
