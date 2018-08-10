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
#include "usdMaya/fallbackPrimReader.h"

#include "usdMaya/translatorUtil.h"

#include "pxr/usd/usdGeom/imageable.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_FallbackPrimReader::UsdMaya_FallbackPrimReader(
    const UsdMayaPrimReaderArgs& args)
    : UsdMayaPrimReader(args)
{
}

bool
UsdMaya_FallbackPrimReader::Read(UsdMayaPrimReaderContext* context)
{
    const UsdPrim& usdPrim = _GetArgs().GetUsdPrim();
    if (usdPrim.HasAuthoredTypeName() && !usdPrim.IsA<UsdGeomImageable>()) {
        // Only create fallback Maya nodes for untyped prims or imageable prims
        // that have no prim reader.
        return false;
    }

    MObject parentNode = context->GetMayaNode(
            usdPrim.GetPath().GetParentPath(), true);

    MStatus status;
    MObject mayaNode;
    return UsdMayaTranslatorUtil::CreateDummyTransformNode(
            usdPrim,
            parentNode,
            /*importTypeName*/ false,
            _GetArgs(),
            context,
            &status,
            &mayaNode);
}

/* static */
UsdMayaPrimReaderRegistry::ReaderFactoryFn
UsdMaya_FallbackPrimReader::CreateFactory()
{
    return [](const UsdMayaPrimReaderArgs& args) {
        return std::make_shared<UsdMaya_FallbackPrimReader>(args);
    };
}

PXR_NAMESPACE_CLOSE_SCOPE
