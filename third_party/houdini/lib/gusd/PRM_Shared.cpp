//
// Copyright 2017 Pixar
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
#include "gusd/PRM_Shared.h"

#include <OP/OP_Node.h>
#include <PRM/PRM_AutoDeleter.h>
#include <PRM/PRM_Parm.h>
#include <PRM/PRM_SpareData.h>
#include <UT/UT_Singleton.h>
#include <UT/UT_WorkBuffer.h>
#include <UT/UT_Version.h>

#include "gusd/stageCache.h"
#include "gusd/GU_USD.h"
#include "gusd/USD_StdTraverse.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_Utils.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/imageable.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {


UT_SingletonWithLock<GusdPRM_Shared::Components>    _componentsSingleton;


void _GenUsdPrimMenu(void* data, PRM_Name* names, int size,
                     const PRM_SpareData* spare, const PRM_Parm* parm)
{
    /** BUG: Having a large number of items in the menu causes items
        to overlap each other in the menu, making the menu unusable.
        Clamp the range as a temporary fix. When real hierarchic menus come
        in, this limit should go away. */
    size = std::min(size, 500);

    names[0] = PRM_Name();
    if(!spare)
        return;
    if(const char* fileParm = spare->getValue("fileprm"))
    {
        OP_Node* node = static_cast<OP_Node*>(data);
        UT_String file;
        node->evalString(file, fileParm, 0, 0);

        GusdStageCacheReader cache;
        if(auto stage = cache.FindOrOpen(UT_StringHolder(file),
                                         GusdStageOpts::LoadNone()))
        {
            GusdPurposeSet purposes = GUSD_PURPOSE_NONE;
            UT_Array<UsdPrim> prims;
            // Only list components (keep the list size small)
            GusdUSD_StdTraverse::GetRecursiveModelTraversal().FindPrims(
                stage->GetPseudoRoot(), UsdTimeCode::Default(), purposes, prims);

            names[0] = PRM_Name("/", "<ROOT>");
            exint primEnd = size - 1; // leave room for end marker.
            int i = 0;
            for(const auto& prim : prims) {
                if(++i > primEnd)
                    break;
                const char* path = prim.GetPath().GetString().c_str();
                names[i] = PRM_Name(path,path);
                names[i].harden();
            }
            names[i] = PRM_Name();
        }
    }
}


void _GenUsdPrimAttrMenu(void* data, PRM_Name* names, int size,
                         const PRM_SpareData* spare, const PRM_Parm* parm)
{
    /** BUG: Same as in _GenUsdPrimMenu() */
    size = std::min(size, 500);

    UT_IntArray idxs;
    parm->getMultiInstanceIndex(idxs);

    names[0] = PRM_Name();
    if(!spare)
        return;
    UT_String fileParm(spare->getValue("fileprm"));
    parm->instanceMultiString(fileParm, idxs, false);
    UT_String primPathParm(spare->getValue("primpathprm"));
    parm->instanceMultiString(primPathParm, idxs, false);

    if(!fileParm.isstring() || !primPathParm.isstring())
        return;

    OP_Node* node = static_cast<OP_Node*>(data);
    UT_String file, primPath;
    node->evalString(file, fileParm, 0, 0);
    node->evalString(primPath, primPathParm, 0, 0);

    GusdStageCacheReader cache;
    UsdPrim usdPrim = cache.GetPrimWithVariants(fileParm, primPath).first;
    if(!usdPrim)
        return;

    std::vector<std::string> keyNames;

    bool wantAttrs=false;
    UT_String primAttrCondition(spare->getValue("primattrcondition"));
    if(primAttrCondition.isstring())
    {
        parm->instanceMultiString(primAttrCondition, idxs, false);
        PRM_Conditional cond(primAttrCondition);
#if UT_MAJOR_VERSION_INT >= 16 
        wantAttrs = cond.eval(*parm, *node->getParmList(), NULL);
#else
        wantAttrs = cond.eval(*node->getParmList(), NULL);
#endif
    }
    if(!primAttrCondition)
        wantAttrs = true;
    if(wantAttrs)
    {
        // XXX: This may wish to examine all attributes (via GetAttributes())
        // rather than just the authored attributes.
        for(const auto& attr : usdPrim.GetAuthoredAttributes())
            keyNames.push_back(attr.GetName());
    }
    std::sort(keyNames.begin(), keyNames.end());
    int maxKeys = std::min((int)keyNames.size(), size-1);
    for(int i = 0; i < maxKeys; ++i)
    {
        names[i] = PRM_Name(keyNames[i].c_str(), keyNames[i].c_str());
        names[i].harden();
    }
    names[maxKeys] = PRM_Name();
}


void _MakePrefixedName(UT_String& str, const char* name,
                       int prefixCount, const char* prefix)
{
    const int len = strlen(prefix);

    UT_WorkBuffer buf;
    for(int i = 0; i < prefixCount; ++i)
        buf.append(prefix, len);
    buf.append(name);
    buf.stealIntoString(str);
}


void _AppendTypes(const TfType& type, UT_Array<PRM_Name>& names,
                  PRM_AutoDeleter& deleter, int depth=0)
{
    const auto& typeName = type.GetTypeName();
    // Add spacing at front, by depth, to indicate hierarchy.
    UT_String label;
    _MakePrefixedName(label, typeName.c_str(), depth, "|   ");
    
    names.append(PRM_Name(typeName.c_str(), deleter.appendCallback(
#if HDK_API_VERSION >= 16050000
	hboost::function<void (char *)>(free), label.steal())));
#else
	boost::function<void (char *)>(free), label.steal())));
#endif
    
    for(const auto& derived : type.GetDirectlyDerivedTypes())
        _AppendTypes(derived, names, deleter, depth+1);
}


PRM_Name* _GetTypeNames()
{
    static UT_Array<PRM_Name> names;
    static PRM_AutoDeleter deleter;

    TfType type = TfType::Find<UsdSchemaBase>();
    _AppendTypes(type, names, deleter);
    
    names.append(PRM_Name());
    
    return &names(0);
}


void _AppendKinds(const GusdUSD_Utils::KindNode* kind,
                  UT_Array<PRM_Name>& names,
                  PRM_AutoDeleter& deleter, int depth=0)
{
    const auto& name = kind->kind.GetString();
    // Add spacing at front, by depth, to indicate hierarchy.
    UT_String label;
    _MakePrefixedName(label, name.c_str(), depth, "|   ");

    names.append(PRM_Name(name.c_str(), deleter.appendCallback(
#if HDK_API_VERSION >= 16050000
	hboost::function<void (char *)>(free), label.steal())));
#else
	boost::function<void (char *)>(free), label.steal())));
#endif
    
    for(const auto& derived : kind->children)
        _AppendKinds(derived.get(), names, deleter, depth+1);
}


PRM_Name* _GetModelKindNames()
{
    static UT_Array<PRM_Name> names;
    static PRM_AutoDeleter deleter;

    for(const auto& kind : GusdUSD_Utils::GetModelKindHierarchy().children)
        _AppendKinds(kind.get(), names, deleter);

    names.append(PRM_Name());

    return &names(0);
}


PRM_Name* _GetPurposeNames()
{
    static UT_Array<PRM_Name> names;
    for(const auto& p : UsdGeomImageable::GetOrderedPurposeTokens())
        names.append(PRM_Name(p.GetString().c_str()));
    names.append(PRM_Name());
    return &names(0);
}


} /*namespace*/



GusdPRM_Shared::GusdPRM_Shared()
    : _components(_componentsSingleton.get())
{}


GusdPRM_Shared::Components::Components() :
    filePattern("*.usd,*.usda,*.usdb,*.usdc"),
    usdFileROData(
        PRM_SpareArgs()
        << PRM_SpareToken(PRM_SpareData::getFileChooserPatternToken(),
                          filePattern)
        << PRM_SpareToken(PRM_SpareData::getFileChooserModeToken(), 
                          PRM_SpareData::getFileChooserModeValRead())),

    usdFileRWData(
        PRM_SpareArgs()
        << PRM_SpareToken(PRM_SpareData::getFileChooserPatternToken(),
                          filePattern)
        << PRM_SpareToken(PRM_SpareData::getFileChooserModeToken(),
                          PRM_SpareData::getFileChooserModeValReadAndWrite())),
    
    usdFileWOData(
        PRM_SpareArgs()
        << PRM_SpareToken(PRM_SpareData::getFileChooserPatternToken(),
                          filePattern)
        << PRM_SpareToken(PRM_SpareData::getFileChooserModeToken(),
                          PRM_SpareData::getFileChooserModeValWrite())),

    filePathName("file", "USD File"),

    primPathName("primpath", "Prim Path"),
    primMenu(PRM_CHOICELIST_REPLACE, _GenUsdPrimMenu),
    multiPrimMenu(PRM_CHOICELIST_TOGGLE, _GenUsdPrimMenu),
    primAttrMenu(PRM_CHOICELIST_REPLACE, _GenUsdPrimAttrMenu),

    fileParm(PRM_SpareArgs()
             << PRM_SpareToken( "fileprm", filePathName.getToken() )),
    primAttrData(PRM_SpareArgs()
                 << PRM_SpareToken( "fileprm", filePathName.getToken())
                 << PRM_SpareToken( "primpathprm", primPathName.getToken() )),
    
    typesMenu(PRM_CHOICELIST_TOGGLE, _GetTypeNames()),
    modelKindsMenu(PRM_CHOICELIST_TOGGLE, _GetModelKindNames()),
    purposesMenu(PRM_CHOICELIST_TOGGLE, _GetPurposeNames()),
    pathAttrDefault(0, GUSD_PATH_ATTR),
    primPathAttrDefault(0, GUSD_PRIMPATH_ATTR),
    variantsAttrDefault(0, GUSD_VARIANTS_ATTR)
{}

PXR_NAMESPACE_CLOSE_SCOPE

