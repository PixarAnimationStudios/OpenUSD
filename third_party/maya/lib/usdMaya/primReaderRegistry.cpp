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
#include "pxr/pxr.h"
#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/debugCodes.h"
#include "usdMaya/registryHelper.h"

#include "pxr/base/plug/registry.h"

#include "pxr/usd/usd/schemaBase.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <boost/assign.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (UsdMaya)
        (PrimReader)
);

typedef std::map<TfToken, PxrUsdMayaPrimReaderRegistry::ReaderFactoryFn> _Registry;
static _Registry _reg;

/* static */
void 
PxrUsdMayaPrimReaderRegistry::Register(
        const TfType& t,
        PxrUsdMayaPrimReaderRegistry::ReaderFactoryFn fn)
{
    TfToken tfTypeName(t.GetTypeName());
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
            "Registering UsdMayaPrimReader for TfType %s.\n", tfTypeName.GetText());
    std::pair< _Registry::iterator, bool> insertStatus = 
        _reg.insert(std::make_pair(tfTypeName, fn));
    if (!insertStatus.second) {
        TF_CODING_ERROR("Multiple readers for type %s", tfTypeName.GetText());
        insertStatus.first->second = fn;
    }
}

/* static */
PxrUsdMayaPrimReaderRegistry::ReaderFactoryFn
PxrUsdMayaPrimReaderRegistry::Find(
        const TfToken& usdTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<PxrUsdMayaPrimReaderRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    TfType tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string typeNameStr = tfType.GetTypeName();
    TfToken typeName(typeNameStr);
    ReaderFactoryFn ret = NULL;
    if (TfMapLookup(_reg, typeName, &ret)) {
        return ret;
    }

    static std::vector<TfToken> SCOPE = boost::assign::list_of
        (_tokens->UsdMaya)(_tokens->PrimReader);
    PxrUsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, typeNameStr);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, typeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
                "No usdMaya reader plugin for TfType %s.  No maya plugin.\n", 
                typeName.GetText());
        _reg[typeName] = NULL;
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE

