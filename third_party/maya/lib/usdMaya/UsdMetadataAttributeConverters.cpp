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

/// \file UsdMetadataAttributeConverters.cpp
/// This file registers converters for storing USD-specific metadata, such
/// as model kind, in extra "USD_" attributes on Maya nodes.

#include "pxr/pxr.h"
#include "usdMaya/AttributeConverter.h"
#include "usdMaya/AttributeConverterRegistry.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/registryManager.h" 
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (USD_hidden)
    (USD_instanceable)
    (USD_kind)
    (USD_typeName)
);

// USD_hidden <-> UsdPrim.IsHidden()
TF_REGISTRY_FUNCTION(AttributeConverterRegistry) {
    FunctionalAttributeConverter* converter = new FunctionalAttributeConverter(
        [](const MFnDependencyNode& srcNode, UsdPrim& destPrim,
                const UsdTimeCode) {
            bool hidden = false;
            if (PxrUsdMayaUtil::getPlugValue(srcNode,
                        _tokens->USD_hidden.GetText(), &hidden)) {
                destPrim.SetHidden(hidden);
            }
            return true;
        },
        [](const UsdPrim& srcPrim, MFnDependencyNode& destNode,
                const UsdTimeCode) {
            if (srcPrim.HasAuthoredHidden()) {
                const bool hidden = srcPrim.IsHidden();
                PxrUsdMayaUtil::createNumericAttribute(destNode,
                        _tokens->USD_hidden.GetText(),
                        MFnNumericData::kBoolean);
                PxrUsdMayaUtil::setPlugValue(destNode,
                        _tokens->USD_hidden.GetText(), hidden);
            }
            return true;
        }
    );
    AttributeConverterRegistry::Register(converter);
}

// USD_instanceable <-> UsdPrim.IsInstanceable()
TF_REGISTRY_FUNCTION(AttributeConverterRegistry) {
    FunctionalAttributeConverter* converter = new FunctionalAttributeConverter(
        [](const MFnDependencyNode& srcNode, UsdPrim& destPrim,
                const UsdTimeCode) {
            bool instanceable = false;
            if (PxrUsdMayaUtil::getPlugValue(srcNode,
                        _tokens->USD_instanceable.GetText(), &instanceable)) {
                destPrim.SetInstanceable(instanceable);
            }
            return true;
        },
        [](const UsdPrim& srcPrim, MFnDependencyNode& destNode,
                const UsdTimeCode) {
            if (srcPrim.IsInstanceable()) {
                PxrUsdMayaUtil::createNumericAttribute(destNode,
                        _tokens->USD_instanceable.GetText(),
                        MFnNumericData::kBoolean);
                PxrUsdMayaUtil::setPlugValue(destNode,
                        _tokens->USD_instanceable.GetText(), true);
            }
            return true;
        }
    );
    AttributeConverterRegistry::Register(converter);
}

// USD_kind <-> UsdModelAPI.GetKind()
TF_REGISTRY_FUNCTION(AttributeConverterRegistry) {
    FunctionalAttributeConverter* converter = new FunctionalAttributeConverter(
        [](const MFnDependencyNode& srcNode, UsdPrim& destPrim,
                const UsdTimeCode) {
            MString kind;
            if (PxrUsdMayaUtil::getPlugValue(srcNode,
                        _tokens->USD_kind.GetText(), &kind)) {
                UsdModelAPI model(destPrim);
                TfToken kindToken(kind.asChar());
                model.SetKind(kindToken);
            }
            return true;
        },
        [](const UsdPrim& srcPrim, MFnDependencyNode& destNode,
                const UsdTimeCode) {
            UsdModelAPI model(srcPrim);
            TfToken kind;
            if (model.GetKind(&kind)) {
                PxrUsdMayaUtil::createStringAttribute(destNode,
                        _tokens->USD_kind.GetText());
                MString kindString(kind.GetText());
                PxrUsdMayaUtil::setPlugValue(destNode,
                        _tokens->USD_kind.GetText(), kindString);
            }
            return true;
        }
    );
    AttributeConverterRegistry::Register(converter);
}

// USD_typeName <-> UsdPrim.GetTypeName()
TF_REGISTRY_FUNCTION(AttributeConverterRegistry) {
    FunctionalAttributeConverter* converter = new FunctionalAttributeConverter(
        [](const MFnDependencyNode& srcNode, UsdPrim& destPrim,
                const UsdTimeCode) {
            MString typeName;
            if (PxrUsdMayaUtil::getPlugValue(srcNode,
                        _tokens->USD_typeName.GetText(), &typeName)) {
                TfToken typeNameToken(typeName.asChar());
                destPrim.SetTypeName(typeNameToken);
            }
            return true;
        },
        [](const UsdPrim& srcPrim, MFnDependencyNode& destNode,
                const UsdTimeCode) {
            // XXX Don't know how to roundtrip custom typenames yet.
            return false;
        }
    );
    AttributeConverterRegistry::Register(converter);
}

PXR_NAMESPACE_CLOSE_SCOPE

