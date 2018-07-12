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
#include "usdMaya/primWriterRegistry.h"

#include "usdMaya/debugCodes.h"
#include "usdMaya/functorPrimWriter.h"
#include "usdMaya/registryHelper.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"

#include <boost/assign.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (UsdMaya)
        (PrimWriter)
);

typedef std::map<std::string, PxrUsdMayaPrimWriterRegistry::WriterFactoryFn> _Registry;
static _Registry _reg;

/* static */
void 
PxrUsdMayaPrimWriterRegistry::Register(
        const std::string& mayaTypeName,
        PxrUsdMayaPrimWriterRegistry::WriterFactoryFn fn)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
            "Registering UsdMayaPrimWriter for maya type %s.\n", mayaTypeName.c_str());

    std::pair< _Registry::iterator, bool> insertStatus = 
        _reg.insert(std::make_pair(mayaTypeName, fn));
    if (!insertStatus.second) {
        TF_CODING_ERROR("Multiple writer for type %s", mayaTypeName.c_str());
        insertStatus.first->second = fn;
    }
}

/* static */
void
PxrUsdMayaPrimWriterRegistry::RegisterRaw(
        const std::string& mayaTypeName,
        PxrUsdMayaPrimWriterRegistry::WriterFn fn)
{
    Register(mayaTypeName, PxrUsdMaya_FunctorPrimWriter::CreateFactory(fn));
}

/* static */
PxrUsdMayaPrimWriterRegistry::WriterFactoryFn
PxrUsdMayaPrimWriterRegistry::Find(
        const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<PxrUsdMayaPrimWriterRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    WriterFactoryFn ret = NULL;
    if (TfMapLookup(_reg, mayaTypeName, &ret)) {
        return ret;
    }

    static std::vector<TfToken> SCOPE = boost::assign::list_of
        (_tokens->UsdMaya)(_tokens->PrimWriter);
    PxrUsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, mayaTypeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
                "No usdMaya writer plugin for maya type %s.  No maya plugin found.\n", 
                mayaTypeName.c_str());
        _reg[mayaTypeName] = NULL;
    }
    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE

