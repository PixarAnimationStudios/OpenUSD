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
#include "usdKatana/usdInPluginRegistry.h"

#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include <FnLogging/FnLogging.h>

FnLogSetup("UsdInPluginRegistry");

PXR_NAMESPACE_OPEN_SCOPE


typedef std::map<std::string, std::string> _UsdTypeRegistry;
static _UsdTypeRegistry _usdTypeReg;
static _UsdTypeRegistry _usdTypeSiteReg;

/* static */
void 
PxrUsdKatanaUsdInPluginRegistry::_RegisterUsdType(
        const std::string& tfTypeName,
        const std::string& opName)
{
    _usdTypeReg[tfTypeName] = opName;
}

/* static */
void 
PxrUsdKatanaUsdInPluginRegistry::_RegisterUsdTypeForSite(
        const std::string& tfTypeName,
        const std::string& opName)
{
    _usdTypeSiteReg[tfTypeName] = opName;
}

bool 
_DoFindUsdType(
    const TfToken& usdTypeName,
    std::string* opName,
    const _UsdTypeRegistry& registry)
{
    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here mostly in case we want to walk up the
    // type hierarchy
    TfType tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string typeNameStr = tfType.GetTypeName();
    return TfMapLookup(registry, typeNameStr, opName);
}


/* static */
bool 
PxrUsdKatanaUsdInPluginRegistry::FindUsdType(
        const TfToken& usdTypeName,
        std::string* opName)
{
    return _DoFindUsdType(usdTypeName, opName, _usdTypeReg);
}

/* static */
bool 
PxrUsdKatanaUsdInPluginRegistry::FindUsdTypeForSite(
        const TfToken& usdTypeName,
        std::string* opName)
{
    return _DoFindUsdType(usdTypeName, opName, _usdTypeSiteReg);
}

typedef std::map<TfToken, std::string> _KindRegistry;
static _KindRegistry _kindReg;
static _KindRegistry _kindExtReg;

/* static */
void 
PxrUsdKatanaUsdInPluginRegistry::RegisterKind(
        const TfToken& kind,
        const std::string& opName)
{
    _kindReg[kind] = opName;
}

/* static */
void 
PxrUsdKatanaUsdInPluginRegistry::RegisterKindForSite(
        const TfToken& kind,
        const std::string& opName)
{
    _kindExtReg[kind] = opName;
}

/* static */
bool
PxrUsdKatanaUsdInPluginRegistry::HasKindsForSite()
{
    return _kindExtReg.size() > 0;
}

/* static */
bool 
PxrUsdKatanaUsdInPluginRegistry::_DoFindKind(
        const TfToken& kind,
        std::string* opName,
        const _KindRegistry& reg)
{
    // can cache this if it becomes an issue.
    TfToken currKind = kind;
    while (!currKind.IsEmpty()) {
        if (TfMapLookup(reg, currKind, opName)) {
            return true;
        }
        if (KindRegistry::HasKind(currKind)) {
            currKind = KindRegistry::GetBaseKind(currKind);
        }
        else {
            FnLogWarn(TfStringPrintf("Unknown kind: '%s'", currKind.GetText()));
            return false;
        }
    }

    return false;
}

/* static */
bool 
PxrUsdKatanaUsdInPluginRegistry::FindKind(
        const TfToken& kind,
        std::string* opName)
{
    return _DoFindKind(kind, opName, _kindReg);
}

/* static */
bool 
PxrUsdKatanaUsdInPluginRegistry::FindKindForSite(
        const TfToken& kind,
        std::string* opName)
{
    return _DoFindKind(kind, opName, _kindExtReg);
}

PXR_NAMESPACE_CLOSE_SCOPE

