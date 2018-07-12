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

#include "usdMaya/MayaPrimWriter.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"

#include "usdMaya/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrUsdMayaPrimWriterRegistry
/// \brief Provides functionality to register and lookup USD writer plugins
/// for Maya nodes.
///
/// Use PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, args, ctx) to define a new
/// writer function, or use
/// PXRUSDMAYA_REGISTER_WRITER(mayaTypeName, writerClass) to register a writer
/// class with the registry.
///
/// The plugin is expected to create a prim at <tt>ctx->GetAuthorPath()</tt>.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name and the Maya plugin to
/// load:
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
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If UsdMaya does not ship with a
/// writer plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct PxrUsdMayaPrimWriterRegistry
{
    /// Writer factory function, i.e. a function that creates a prim writer
    /// for the given DAG/USD paths and context.
    typedef std::function< MayaPrimWriterPtr (
            const MDagPath&,
            const SdfPath&,
            usdWriteJobCtx&) > WriterFactoryFn;

    /// Writer function, i.e. a function that writes a prim. This is the
    /// signature of the function defined by the PXRUSDMAYA_DEFINE_WRITER
    /// macro.
    typedef std::function< bool (
            const PxrUsdMayaPrimWriterArgs&,
            PxrUsdMayaPrimWriterContext*) > WriterFn;

    /// \brief Register \p fn as a factory function providing a MayaPrimWriter
    /// subclass that can be used to write \p mayaType.
    /// If you can't provide a valid MayaPrimWriter for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a writer factory in your custom plugin:
    /// \code{.cpp}
    /// class MyWriter : public MayaPrimWriter {
    ///     static MayaPrimWriterPtr Create(
    ///             MDagPath &curDag,
    ///             const SdfPath& uPath,
    ///             usdWriteJobCtx& jobCtx);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimWriterRegistry, MyWriter) {
    ///     PxrUsdMayaPrimWriterRegistry::Register("myCustomMayaNode",
    ///             MyWriter::Create);
    /// }
    /// \endcode
    PXRUSDMAYA_API
    static void Register(const std::string& mayaType, WriterFactoryFn fn);

    /// \brief Wraps \p fn in a WriterFactoryFn and registers the wrapped
    /// function as a prim writer provider.
    /// This is a helper method for the macro PXRUSDMAYA_DEFINE_WRITER;
    /// you probably want to use PXRUSDMAYA_DEFINE_WRITER directly instead.
    PXRUSDMAYA_API
    static void RegisterRaw(const std::string& mayaType, WriterFn fn);

    /// \brief Finds a writer if one exists for \p mayaTypeName.
    ///
    /// If there is no writer plugin for \p mayaTypeName, returns nullptr.
    PXRUSDMAYA_API
    static WriterFactoryFn Find(const std::string& mayaTypeName);
};

// Note, TF_REGISTRY_FUNCTION_WITH_TAG needs a type to register with so we
// create a dummy struct in the macro.

/// \brief Defines a writer function for the given Maya type; the function
/// should write a USD prim for the given Maya node. The return status indicates
/// whether the operation succeeded.
///
/// Example:
/// \code{.cpp}
/// PXRUSDMAYA_DEFINE_WRITER(myCustomMayaNode, args, context) {
///     context->GetUsdStage()->DefinePrim(context->GetAuthorPath());
///     return true;
/// }
/// \endcode
#define PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, argsVarName, ctxVarName) \
struct PxrUsdMayaWriterDummy_##mayaTypeName { }; \
static bool PxrUsdMaya_PrimWriter_##mayaTypeName( \
    const PxrUsdMayaPrimWriterArgs&, \
    PxrUsdMayaPrimWriterContext*); \
TF_REGISTRY_FUNCTION_WITH_TAG( \
    PxrUsdMayaPrimWriterRegistry, \
    PxrUsdMayaWriterDummy_##mayaTypeName) \
{ \
    PxrUsdMayaPrimWriterRegistry::RegisterRaw( \
            #mayaTypeName, PxrUsdMaya_PrimWriter_##mayaTypeName); \
} \
bool PxrUsdMaya_PrimWriter_##mayaTypeName( \
    const PxrUsdMayaPrimWriterArgs& argsVarName, \
    PxrUsdMayaPrimWriterContext* ctxVarName)

/// \brief Registers a pre-existing writer class for the given Maya type;
/// the writer class should be a subclass of MayaPrimWriter with a three-place
/// constructor that takes <tt>(const MDagPath& mayaPath,
/// const SdfPath& usdPath, usdWriteJobCtx& jobCtx)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyWriter : public MayaPrimWriter {
///     MyWriter(
///             const MDagPath& mayaPath,
///             const SdfPath& usdPath,
///             usdWriteJobCtx& jobCtx) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_WRITER(myCustomMayaNode, MyWriter);
/// \endcode
#define PXRUSDMAYA_REGISTER_WRITER(mayaTypeName, writerClass) \
TF_REGISTRY_FUNCTION_WITH_TAG( \
    PxrUsdMayaPrimWriterRegistry, \
    mayaTypeName##_##writerClass) \
{ \
    PxrUsdMayaPrimWriterRegistry::Register(#mayaTypeName, \
        [](const MDagPath& mayaPath, const SdfPath& usdPath, \
                usdWriteJobCtx& jobCtx) { \
            return std::make_shared<writerClass>(mayaPath, usdPath, jobCtx); \
        }); \
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_PRIMWRITERREGISTRY_H

