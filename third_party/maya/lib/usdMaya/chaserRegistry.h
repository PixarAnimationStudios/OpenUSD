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
#ifndef PXRUSDMAYA_CHASER_REGISTRY_H
#define PXRUSDMAYA_CHASER_REGISTRY_H

/// \file usdMaya/chaserRegistry.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/chaser.h"
#include "usdMaya/jobArgs.h"
#include "usdMaya/util.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"

#include <boost/function.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(UsdMayaChaserRegistry);

/// \class UsdMayaChaserRegistry
/// \brief Registry for chaser plugins.
///
/// We allow sites to register new chaser scripts that can be enabled on export.
///
/// Use PXRUSDMAYA_DEFINE_CHASER_FACTORY(name, ctx) to register a new chaser.
///
/// Unfortunately, these are only available through the command/python interface
/// and not yet exposed in the translator interface.
class UsdMayaChaserRegistry : public TfWeakBase
{
public:

    /// \brief Holds data that can be accessed when constructing a 
    /// \p UsdMayaChaser object.
    ///
    /// This class allows plugin code to only know about the context object
    /// during construction and only need to know about the data it is needs to
    /// construct.  
    class FactoryContext {
    public:
        typedef UsdMayaUtil::MDagPathMap<SdfPath> DagToUsdMap;

        PXRUSDMAYA_API
        FactoryContext(
                const UsdStagePtr& stage, 
                const DagToUsdMap& dagToUsdMap,
                const UsdMayaJobExportArgs& jobArgs);

        /// \brief Returns the exported stage.
        ///
        /// It is safe for the \p UsdMayaChaser to save this return value and
        /// use it during it's execution.
        PXRUSDMAYA_API
        UsdStagePtr GetStage() const;

        /// \brief Returns a map that maps full MDagPath's to Usd prim paths.
        ///
        /// It is safe for the \p UsdMayaChaser to save this return value by
        /// reference and use it during it's execution.
        PXRUSDMAYA_API
        const DagToUsdMap& GetDagToUsdMap() const;

        /// \brief Returns the current job args.
        ///
        /// It is safe for the \p UsdMayaChaser to save this return value by
        /// reference and use it during it's execution.
        PXRUSDMAYA_API
        const UsdMayaJobExportArgs& GetJobArgs() const;

    private:
        UsdStagePtr _stage;
        const DagToUsdMap& _dagToUsdMap;
        const UsdMayaJobExportArgs& _jobArgs;
    };

    typedef boost::function<UsdMayaChaser* (const FactoryContext&)> FactoryFn;

    /// \brief Register a chaser factory.  
    ///
    /// Please use the \p PXRUSDMAYA_DEFINE_CHASER_FACTORY instead of calling
    /// this directly.
    PXRUSDMAYA_API
    bool RegisterFactory(
            const std::string& name, 
            FactoryFn fn);

    /// \brief Creates a chaser using the factoring registered to \p name.
    PXRUSDMAYA_API
    UsdMayaChaserRefPtr Create(
            const std::string& name, 
            const FactoryContext& context) const;

    /// \brief Returns the names of all registered chasers.
    PXRUSDMAYA_API
    std::vector<std::string> GetAllRegisteredChasers() const;

    PXRUSDMAYA_API
    static UsdMayaChaserRegistry& GetInstance();

private:
    UsdMayaChaserRegistry();
    ~UsdMayaChaserRegistry();
    friend class TfSingleton<UsdMayaChaserRegistry>;
};

/// \brief define a factory for the chaser \p name.  the \p contextArgName will
/// be type \p UsdMayaChaserRegistry::FactoryContext .  The following code
/// block should return a \p UsdMayaChaser*.  There are no guarantees about
/// the lifetime of \p contextArgName. 
#define PXRUSDMAYA_DEFINE_CHASER_FACTORY(name, contextArgName) \
static UsdMayaChaser* _ChaserFactory_##name(const UsdMayaChaserRegistry::FactoryContext&); \
TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaChaserRegistry, name) {\
    UsdMayaChaserRegistry::GetInstance().RegisterFactory(#name, &_ChaserFactory_##name); \
}\
UsdMayaChaser* _ChaserFactory_##name(const UsdMayaChaserRegistry::FactoryContext& contextArgName)


PXR_NAMESPACE_CLOSE_SCOPE

#endif
