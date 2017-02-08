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
#ifndef PXRUSDMAYA_PRIMREADERREGISTRY_H
#define PXRUSDMAYA_PRIMREADERREGISTRY_H

/// \file primReaderRegistry.h

#include "pxr/pxr.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/base/tf/registryManager.h" 

PXR_NAMESPACE_OPEN_SCOPE


/// \class PxrUsdMayaPrimReaderRegistry
/// \brief Provides functionality to register and lookup usd Maya reader
/// plugins.
///
/// Use PXRUSDMAYA_DEFINE_READER(MyUsdType, args, ctx) to register a new reader
/// for maya.  
///
/// In order for the core system to discover the plugin, you should also
/// have a plugInfo.json file that contains the type and maya plugin to load:
/// \code
/// {
///     "UsdMaya": {
///         "PrimReader": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "MyUsdType"
///             ]
///         }
///     }
/// } 
/// \endcode
struct PxrUsdMayaPrimReaderRegistry
{
    typedef boost::function< bool (
            const PxrUsdMayaPrimReaderArgs&, 
            PxrUsdMayaPrimReaderContext*) > ReaderFn;

    /// \brief Register \p fn as a reader provider for \p type.
    static void Register(const TfType& type, ReaderFn fn);

    /// \brief Register \p fn as a reader provider for \p T.
    template <typename T>
    static void Register(ReaderFn fn)
    {
        if (TfType t = TfType::Find<T>()) {
            Register(t, fn);
        }
        else {
            TF_CODING_ERROR("Cannot register unknown TfType: %s.",
                    ArchGetDemangled<T>().c_str());
        }
    }

    // takes a usdType (i.e. prim.GetTypeName())
    /// \brief Finds a reader if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// prim.GetTypeName()
    /// \endcode
    static ReaderFn Find(
            const TfToken& usdTypeName);

};

#define PXRUSDMAYA_DEFINE_READER(T, argsVarName, ctxVarName)\
static bool PxrUsdMaya_PrimReader_##T(const PxrUsdMayaPrimReaderArgs&, PxrUsdMayaPrimReaderContext*); \
TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimReaderRegistry, T) \
{\
    PxrUsdMayaPrimReaderRegistry::Register<T>(PxrUsdMaya_PrimReader_##T);\
}\
bool PxrUsdMaya_PrimReader_##T(const PxrUsdMayaPrimReaderArgs& argsVarName, PxrUsdMayaPrimReaderContext* ctxVarName)


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_PRIMREADERREGISTRY_H
