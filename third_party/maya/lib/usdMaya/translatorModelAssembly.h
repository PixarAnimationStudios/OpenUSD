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
#ifndef PXRUSDMAYA_TRANSLATOR_MODELASSEMBLY_H
#define PXRUSDMAYA_TRANSLATOR_MODELASSEMBLY_H

/// \file translatorModelAssembly.h

#include "usdMaya/api.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MObject.h>

#include <map>
#include <string>


struct USDMAYA_API PxrUsdMayaTranslatorModelAssembly
{
    /// This method generates a USD prim with a model reference
    /// when provided args and a context that identify a Maya
    /// assembly node.
    static bool Create(
        const PxrUsdMayaPrimWriterArgs& args, 
        PxrUsdMayaPrimWriterContext* context);

    /// This method returns true if \p prim being considered for import under
    /// \p usdImportRootPrim should be imported into Maya as an assembly.
    /// XXX: This might be a candidate for a plugin point that studios would
    //  want to customize.
    static bool ShouldImportAsAssembly(
        const UsdPrim& usdImportRootPrim,
        const UsdPrim& prim);

    /// Imports the model at \p prim as a new Maya assembly under
    /// \p parentNode. An assembly node of type \p assemblyTypeName will be
    /// created, and if \p assemblyRep is non-empty, that representation will
    /// be activated after creation.
    /// Returns true if this succeeds in creating an assembly for \p prim.
    static bool Read(
        const UsdPrim& prim,
        const std::string& assetIdentifier,
        const SdfPath& assetPrimPath,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        const std::string& assemblyTypeName,
        const std::string& assemblyRep);

    /// Creates a Maya USD proxy shape node for the USD prim \p prim under
    /// \p parentNode. A node of type \p proxyShapeTypeName will be created.
    /// Returns true if this succeeds in creating a proxy shape for \p prim.
    static bool ReadAsProxy(
        const UsdPrim& prim,
        const std::map<std::string, std::string>& variantSetSelections,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        const std::string& proxyShapeTypeName);
};


#endif // PXRUSDMAYA_TRANSLATOR_MODELASSEMBLY_H
