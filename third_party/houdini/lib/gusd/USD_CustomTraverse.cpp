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
#include "gusd/USD_CustomTraverse.h"

#include <OP/OP_Parameters.h>
#include <PRM/PRM_Shared.h>

#include "gusd/PRM_Shared.h"
#include "gusd/USD_ThreadedTraverse.h"
#include "gusd/USD_Utils.h"

#include "pxr/base/plug/registry.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usdGeom/imageable.h"

PXR_NAMESPACE_OPEN_SCOPE

void
GusdUSD_CustomTraverse::Opts::Reset()
{
    defined = TRUE_STATE;
    abstract = FALSE_STATE;
    active = TRUE_STATE;
    visible = TRUE_STATE;
    imageable = TRUE_STATE;
    model = group = instance = master = clips = ANY_STATE;

    traverseMatched = false;
    kinds.clear();
    purposes.clear();
    types.clear();
}


namespace {

void _PredicateSwitch(Usd_PrimFlagsConjunction& p,
                      GusdUSD_CustomTraverse::TriState state,
                      Usd_PrimFlags flag)
{
    switch(state) {
    case GusdUSD_CustomTraverse::TRUE_STATE:  p &= flag; break;
    case GusdUSD_CustomTraverse::FALSE_STATE: p &= !flag; break;
    default: break;
    }
}

} /*namespace*/


Usd_PrimFlagsPredicate
GusdUSD_CustomTraverse::Opts::MakePredicate() const
{
    // Build a predicate from the user-configured options.

    /* Note that we *intentionally* exclude load state from being
       user-configurable, since traversers are primarily intended to
       be used on pure, read-only caches, in which case users aren't meant
       to know about prim load states.

       We also don't default add UsdPrimIsLoaded at all to the predicate,
       as that prevents users from traversing to inactive prims, since
       if a prim carrying payloads has been deactivated, the prim will
       be considered both inactive and unloaded.*/

    Usd_PrimFlagsConjunction p;

    _PredicateSwitch(p, active,     UsdPrimIsActive);
    _PredicateSwitch(p, model,      UsdPrimIsModel);
    _PredicateSwitch(p, group,      UsdPrimIsGroup);
    _PredicateSwitch(p, defined,    UsdPrimIsDefined);
    _PredicateSwitch(p, abstract,   UsdPrimIsAbstract);
    _PredicateSwitch(p, instance,   UsdPrimIsInstance);
    _PredicateSwitch(p, master,     Usd_PrimMasterFlag);
    _PredicateSwitch(p, clips,      Usd_PrimClipsFlag);
    return p;
}


bool
GusdUSD_CustomTraverse::Opts::Configure(OP_Parameters& parms, fpreal t)
{
#define _EVALTRI(NAME)                                          \
    NAME = static_cast<TriState>(parms.evalInt(#NAME, 0, t));

    _EVALTRI(active);
    _EVALTRI(visible);
    _EVALTRI(imageable);
    _EVALTRI(defined);
    _EVALTRI(abstract);
    _EVALTRI(model);
    _EVALTRI(group);
    _EVALTRI(instance);
    _EVALTRI(master);
    _EVALTRI(clips);


    traverseMatched = parms.evalInt("traversematched", 0, t);

#define _EVALSTR(NAME,VAR)  parms.evalString(VAR, #NAME, 0, t);

    UT_String kindsStr, purposesStr, typesStr;
    _EVALSTR(kinds, kindsStr);
    _EVALSTR(purposes, purposesStr);
    _EVALSTR(types, typesStr);

    std::string err;
    if(!SetKindsByPattern(kindsStr, true, &err) ||
       !SetPurposesByPattern(purposesStr, true, &err) ||
       !SetTypesByPattern(typesStr, true, &err)) {

        parms.opLocalError(OP_ERR_ANYTHING, err.c_str());
        return false;
    }

    UT_String namePatternStr, pathPatternStr;
    _EVALSTR(namemask, namePatternStr);
    _EVALSTR(pathmask, pathPatternStr);

    SetNamePattern(namePatternStr);
    SetPathPattern(pathPatternStr);

    if(kinds.size() > 0 && model == FALSE_STATE) {
        parms.opLocalError(OP_ERR_ANYTHING,
                          "Model kinds specified, but models "
                          "are being excluded. Matches are impossible.");
        return false;
    }
    return true;
}


namespace {

void
_BadPatternError(const char* type,
                 const char* pattern,
                 std::string* err=NULL)
{
    if(err) {
        UT_WorkBuffer buf;
        buf.sprintf("No %s matched pattern '%s'", type, pattern);
        *err = buf.toStdString();
    }
}


void
_SetPattern(UT_StringMMPattern& patternObj,
            const char* pattern,
            bool caseSensitive)
{
    if(!UTisstring(pattern) || UT_String(pattern) == "*") {
        patternObj.clear();
    } else {
        patternObj.compile(pattern, caseSensitive);
    }
}
            


} /*namespace*/


bool
GusdUSD_CustomTraverse::Opts::SetKindsByPattern(
    const char* pattern, bool caseSensitive, std::string* err)
{
    if(!UTisstring(pattern) || UT_String(pattern) == "*") { 
        kinds.clear();
        return true;
    }
    GusdUSD_Utils::GetBaseModelKindsMatchingPattern(
        pattern, kinds, caseSensitive);
    if(kinds.size() == 0) {
        _BadPatternError("model kinds", pattern, err);
        return false;
    }
    return true;
}


bool
GusdUSD_CustomTraverse::Opts::SetPurposesByPattern(
    const char* pattern, bool caseSensitive, std::string* err)
{
    if(!UTisstring(pattern) || UT_String(pattern) == "*") {
        // Empty pattern means 'match everything'
        purposes.clear();
        return true;
    }
    GusdUSD_Utils::GetPurposesMatchingPattern(pattern, purposes, caseSensitive);
    if(purposes.size() == 0) {
        _BadPatternError("purposes", pattern, err);
        return false;
    }
    return true;
}


bool
GusdUSD_CustomTraverse::Opts::SetTypesByPattern(
    const char* pattern, bool caseSensitive, std::string* err)
{
    if(!UTisstring(pattern) || UT_String(pattern) == "*") { 
        // Empty pattern means 'match everything'
        types.clear();
        return true;
    }
    GusdUSD_Utils::GetBaseSchemaTypesMatchingPattern(pattern, types,
                                                     caseSensitive);
    if(types.size() == 0) {
        _BadPatternError("prim schema types", pattern, err);
        return false;
    }
    return true;
}


void
GusdUSD_CustomTraverse::Opts::SetNamePattern(
    const char* pattern, bool caseSensitive)
{
    _SetPattern(namePattern, pattern, caseSensitive);
}


void
GusdUSD_CustomTraverse::Opts::SetPathPattern(
    const char* pattern, bool caseSensitive)
{
    _SetPattern(pathPattern, pattern, caseSensitive);
}


namespace {


struct _Visitor
{
    _Visitor(const GusdUSD_CustomTraverse::Opts& opts)
        : _opts(opts),
          _predicate(opts.MakePredicate()) {}

    Usd_PrimFlagsPredicate  TraversalPredicate() const
                            {
                                // Need a predicate matching all prims.
                                return Usd_PrimFlagsPredicate::Tautology();
                            }

    bool                    AcceptPrim(const UsdPrim& prim,
                                       UsdTimeCode time,
                                       GusdPurposeSet purposes,
                                       GusdUSD_TraverseControl& ctl) const;
    
    bool                    AcceptType(const UsdPrim& prim) const;

    bool                    AcceptPurpose(const UsdGeomImageable& prim) const;

    bool                    AcceptKind(const UsdPrim& prim) const;

    bool                    AcceptVis(const UsdGeomImageable& prim,
                                      UsdTimeCode time) const;

    bool                    AcceptNamePattern(const UsdPrim& prim) const;

    bool                    AcceptPathPattern(const UsdPrim& prim) const;

private:
    const GusdUSD_CustomTraverse::Opts& _opts;
    const Usd_PrimFlagsPredicate        _predicate;
};


bool
_Visitor::AcceptPrim(const UsdPrim& prim,
                     UsdTimeCode time,
                     GusdPurposeSet purposes,
                     GusdUSD_TraverseControl& ctl) const
{
    UsdGeomImageable ip(prim);

    bool visit = true;

    if(BOOST_UNLIKELY(!(bool)ip)) {
        // Prim is not imageable
        if(_opts.imageable == GusdUSD_CustomTraverse::TRUE_STATE) {
            visit = false;
            // will be inherited.
            ctl.PruneChildren();
        } else if(_opts.purposes.size() > 0 ||
                  _opts.visible == GusdUSD_CustomTraverse::TRUE_STATE) {
            // Can only match prims that depend on imageable attributes
            // Since this prim is not imageable, it can't possibly
            // match our desired visibility or purpose.
            visit = false;
        }
    }
            
    /* These tests are based on cached data;
       check them before anything that requires attribute reads.*/
    visit = visit && _predicate(prim) && AcceptType(prim);
    
    visit = visit && AcceptVis(ip, time)
                  && AcceptPurpose(ip)
                  && AcceptKind(prim)
                  && AcceptNamePattern(prim)
                  && AcceptPathPattern(prim);

    if(visit && !_opts.traverseMatched)
        ctl.PruneChildren();

    return visit;
}


bool
_Visitor::AcceptType(const UsdPrim& prim) const
{
    if(_opts.types.size() == 0)
        return true;

    const std::string& typeName = prim.GetTypeName().GetString();
    if(!typeName.empty()) {
        /* TODO: profile this search.
                 It may be faster to fill an unordered set of type
                 names to do this test instead.*/
        TfType type =
            PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(typeName);
        for(auto& t : _opts.types) {
            if(type.IsA(t)) {
                return true;
            }
        }
    }
    return false;
}


bool
_Visitor::AcceptPurpose(const UsdGeomImageable& prim)const
{
    if(_opts.purposes.size() == 0)
        return true;

    TfToken purpose;
    prim.GetPurposeAttr().Get(&purpose);
    for(auto& p : _opts.purposes) {
        if(p == purpose)
            return true;
    }
    return false;
}


bool
_Visitor::AcceptKind(const UsdPrim& prim) const
{
    if(_opts.kinds.size() == 0)
        return true;

    UsdModelAPI model(prim);
    TfToken kind;
    model.GetKind(&kind);
    for(auto& k : _opts.kinds) {
        if(KindRegistry::IsA(kind, k))
            return true;
    }
    return false;
}


bool
_Visitor::AcceptVis(const UsdGeomImageable& prim, UsdTimeCode time) const
{
    if(_opts.visible == GusdUSD_CustomTraverse::ANY_STATE)
        return true;

    bool vis = GusdUSD_Utils::ImageablePrimIsVisible(prim, time);
    if(_opts.visible == GusdUSD_CustomTraverse::TRUE_STATE)
        return vis;
    return !vis;
}


bool
_Visitor::AcceptNamePattern(const UsdPrim& prim) const
{
    if(_opts.namePattern.isEmpty())
        return true;
    return UT_String(prim.GetName().GetText()).multiMatch(_opts.namePattern);
}


bool
_Visitor::AcceptPathPattern(const UsdPrim& prim) const
{
    if(_opts.pathPattern.isEmpty())
        return true;
    return UT_String(prim.GetPath().GetText()).multiMatch(_opts.pathPattern);
}


static GusdUSD_CustomTraverse::Opts _defaultOpts;


} /*namespaces*/


bool
GusdUSD_CustomTraverse::FindPrims(const UsdPrim& root,
                                  UsdTimeCode time,
                                  GusdPurposeSet purposes,
                                  UT_Array<UsdPrim>& prims,
                                  bool skipRoot,
                                  const GusdUSD_Traverse::Opts* opts) const
{
    const auto* customOpts = UTverify_cast<const Opts*>(opts);
    _Visitor visitor(customOpts ? *customOpts : _defaultOpts);

    return GusdUSD_ThreadedTraverse::ParallelFindPrims(
        root, time, purposes, prims, visitor, skipRoot);
}


bool
GusdUSD_CustomTraverse::FindPrims(
    const UT_Array<UsdPrim>& roots,
    const GusdDefaultArray<UsdTimeCode>& times,
    const GusdDefaultArray<GusdPurposeSet>& purposes,
    UT_Array<PrimIndexPair>& prims,
    bool skipRoot,
    const GusdUSD_Traverse::Opts* opts) const
{
    const auto* customOpts = UTverify_cast<const Opts*>(opts);
    _Visitor visitor(customOpts ? *customOpts : _defaultOpts);

    return GusdUSD_ThreadedTraverse::ParallelFindPrims(
        roots, times, purposes, prims, visitor, skipRoot);
}


namespace {


const PRM_Template* _CreateTemplates()
{
    static PRM_Default trueDef(GusdUSD_CustomTraverse::TRUE_STATE, "");
    static PRM_Default falseDef(GusdUSD_CustomTraverse::FALSE_STATE, "");
    static PRM_Default anyDef(GusdUSD_CustomTraverse::ANY_STATE, "");

    static PRM_Name activeName("active", "Is Active");
    static PRM_Name visibleName("visible", "Is Visible");
    static PRM_Name imageableName("imageable", "Is Imageable");
    static PRM_Name definedName("defined", "Is Defined");
    static PRM_Name abstractName("abstract", "Is Abstract");
    static PRM_Name groupName("group", "Is Group");
    static PRM_Name modelName("model", "Is Model");
    static PRM_Name instanceName("instance", "Is Instance");
    static PRM_Name masterName("master", "Is Instance Master");
    static PRM_Name clipsName("clips", "Has Clips");

    static PRM_Name stateNames[] = {
        PRM_Name("true", "True"),
        PRM_Name("false", "False"),
        PRM_Name("any", "Ignore"),
        PRM_Name()
    };
    static PRM_ChoiceList stateMenu(PRM_CHOICELIST_SINGLE, stateNames);

    static PRM_Name nameMaskName("namemask", "Name Mask");
    static PRM_Name pathMaskName("pathmask", "Path Mask");

    static PRM_Name traverseMatchedName("traversematched", "Traverse Matched");
    
    static PRM_Name typesName("types", "Prim Types");
    static PRM_Name purposesName("purposes", "Purposes");
    static PRM_Name kindsName("kinds", "Kinds");

#define _STATETEMPLATE(name,def)                        \
    PRM_Template(PRM_ORD, 1, &name, &def, &stateMenu)
    
    GusdPRM_Shared shared;

    static PRM_Template templates[] = {
        PRM_Template(PRM_STRING, 1, &typesName,
                     PRMzeroDefaults, &shared->typesMenu),
        PRM_Template(PRM_STRING, 1, &purposesName,
                     PRMzeroDefaults, &shared->purposesMenu),
        PRM_Template(PRM_STRING, 1, &kindsName,
                     PRMzeroDefaults, &shared->modelKindsMenu),
        PRM_Template(PRM_STRING, 1, &nameMaskName, PRMzeroDefaults),
        PRM_Template(PRM_STRING, 1, &pathMaskName, PRMzeroDefaults),
        PRM_Template(PRM_TOGGLE, 1, &traverseMatchedName, PRMzeroDefaults),
        
        _STATETEMPLATE(activeName, trueDef),
        _STATETEMPLATE(visibleName, trueDef),
        _STATETEMPLATE(imageableName, trueDef),
        _STATETEMPLATE(definedName, trueDef),
        _STATETEMPLATE(abstractName, falseDef),
        _STATETEMPLATE(groupName, anyDef),
        _STATETEMPLATE(modelName, anyDef),
        _STATETEMPLATE(instanceName, anyDef),
        _STATETEMPLATE(masterName, anyDef),
        _STATETEMPLATE(clipsName, anyDef),
        PRM_Template()
    };
    return templates;
}

GusdUSD_TraverseType _type(new GusdUSD_CustomTraverse,
                           "std:custom", "Custom Traversal", _CreateTemplates(),
                           "Configurable traversal, allowing complex "
                           "discovery patterns.");

} /*namespace*/

PXR_NAMESPACE_CLOSE_SCOPE

