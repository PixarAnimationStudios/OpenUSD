//
// Copyright 2018 Pixar
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

/// \file UsdGeomAttributeConverters.cpp
/// This file registers converters for storing USDGeom-specific metadata, such
/// as purpose, in extra "USDGeom_" attributes on Maya nodes.

#include "pxr/pxr.h"
#include "usdMaya/AttributeConverter.h"
#include "usdMaya/AttributeConverterRegistry.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/registryManager.h" 
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/imageable.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (USD_purpose)
);

// USD_purpose <-> UsdGeomImageable.GetPurposeAttr()
TF_REGISTRY_FUNCTION(AttributeConverterRegistry) {
    FunctionalAttributeConverter* converter = new FunctionalAttributeConverter(
        [](const MFnDependencyNode& srcNode, UsdPrim& destPrim,
                const UsdTimeCode) {
            if (!destPrim.IsA<UsdGeomImageable>()) { return true; }
            MString purpose;
            if (PxrUsdMayaUtil::getPlugValue(srcNode,
                        _tokens->USD_purpose.GetText(), &purpose)) {
                UsdGeomImageable imageable(destPrim);
                if (imageable) {
                    TfToken purposeToken(purpose.asChar());
                    imageable.GetPurposeAttr().Set(purposeToken);
                }
            }
            return true;
        },
        [](const UsdPrim& srcPrim, MFnDependencyNode& destNode,
                const UsdTimeCode) {
            if (!srcPrim.IsA<UsdGeomImageable>()) { return true; }
            UsdGeomImageable imageable(srcPrim);
            if (!imageable) { return true; }
            const auto purposeAttr = imageable.GetPurposeAttr();
            if (!purposeAttr.HasAuthoredValueOpinion()) { return true; }
            TfToken purpose;
            if (purposeAttr.Get(&purpose)) {
                PxrUsdMayaUtil::createStringAttribute(destNode,
                        _tokens->USD_purpose.GetText());
                MString purposeString(purpose.GetText());
                PxrUsdMayaUtil::setPlugValue(destNode,
                        _tokens->USD_purpose.GetText(), purposeString);
            }
            return true;
        }
    );
    AttributeConverterRegistry::Register(converter);
}

PXR_NAMESPACE_CLOSE_SCOPE

