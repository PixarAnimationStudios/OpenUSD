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
#ifndef PXRUSDMAYA_PRIMWRITERREGISTRY_H
#define PXRUSDMAYA_PRIMWRITERREGISTRY_H

/// \file primWriterRegistry.h

#include "pxr/pxr.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"

#include <boost/function.hpp>

PXR_NAMESPACE_OPEN_SCOPE


/// \class PxrUsdMayaPrimWriterRegistry
/// \brief Provides functionality to register and lookup usd Maya writer
/// plugins.
///
/// Use PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, args, ctx) to register a new
/// writer for maya.  The plugin is expected to create a prim at
///
/// \code
/// ctx->GetAuthorPath()
/// \endcode
///
/// In order for the core system to discover the plugin, you need a
/// plugInfo.json that contains the maya type name and the maya plugin to load:
/// \code
/// {
///     "UsdMaya": {
///         "PrimWriter": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "myMayaType"
///             ]
///         }
///     }
/// } 
/// \endcode
///
/// For now, there is only support for user-defined shape plugins, with more
/// general support to come in the future.
struct PxrUsdMayaPrimWriterRegistry
{
    typedef boost::function< bool (
            const PxrUsdMayaPrimWriterArgs&,
            PxrUsdMayaPrimWriterContext*) > WriterFn;

    /// \brief Register \p fn as a writer provider for \p mayaType.
    static void Register(const std::string& mayaType, WriterFn fn);

    /// \brief Finds a writer if one exists for \p mayaTypeName.
    ///
    /// If there is no writer plugin for \p mayaTypeName, this will return 
    /// a value that evaluates to false.
    static WriterFn Find(const std::string& mayaTypeName);
};

// Note, TF_REGISTRY_FUNCTION_WITH_TAG needs a type to register with so we
// create a dummy struct in the macro.
#define PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, argsVarName, ctxVarName)\
struct PxrUsdMayaWriterDummy_##mayaTypeName { }; \
static bool PxrUsdMaya_PrimWriter_##mayaTypeName(const PxrUsdMayaPrimWriterArgs&, PxrUsdMayaPrimWriterContext*); \
TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimWriterRegistry, PxrUsdMayaWriterDummy_##mayaTypeName) \
{\
    PxrUsdMayaPrimWriterRegistry::Register(#mayaTypeName, PxrUsdMaya_PrimWriter_##mayaTypeName);\
}\
bool PxrUsdMaya_PrimWriter_##mayaTypeName(const PxrUsdMayaPrimWriterArgs& argsVarName, PxrUsdMayaPrimWriterContext* ctxVarName)


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_PRIMWRITERREGISTRY_H

