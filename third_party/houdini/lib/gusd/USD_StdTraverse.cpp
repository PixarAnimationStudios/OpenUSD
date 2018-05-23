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
#include "gusd/USD_StdTraverse.h"

#include "gusd/USD_Traverse.h"
#include "gusd/USD_TraverseSimple.h"

#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/mesh.h"

#include "pxr/usd/usdLux/light.h"

PXR_NAMESPACE_OPEN_SCOPE

using GusdUSD_ThreadedTraverse::DefaultImageablePrimVisitorT;

namespace GusdUSD_StdTraverse {

namespace {


template <class Type>
struct _VisitByTypeT
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl) const
            {
                if(BOOST_UNLIKELY((bool)Type(prim))) {
                    ctl.PruneChildren();
                    return true;
                }
                return false;
            }
};


typedef DefaultImageablePrimVisitorT<
    _VisitByTypeT<UsdGeomMesh> >        _VisitImageableMeshes;
typedef DefaultImageablePrimVisitorT<
    _VisitByTypeT<UsdGeomGprim> >   _VisitImageableGprims;
typedef DefaultImageablePrimVisitorT<
    _VisitByTypeT<UsdGeomBoundable> >   _VisitImageableBoundables;


struct _VisitBoundablesAndInstances
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl) const
            {
                if(BOOST_UNLIKELY((bool)UsdGeomBoundable(prim) || prim.IsInstance())) {
                    ctl.PruneChildren();
                    return true;
                }
                return false;
            } 
};   

typedef DefaultImageablePrimVisitorT<
    _VisitBoundablesAndInstances,true>   _VisitImageableBoundablesAndInstances;

struct _VisitModels
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
            {
                if(BOOST_UNLIKELY(prim.IsModel())) {
                    UsdModelAPI model(prim);
                    if(model) {
                        TfToken kind;
                        model.GetKind(&kind);
                        if(KindRegistry::IsA(kind, KindTokens->component)) {
                            // No models can appear beneath components.
                            ctl.PruneChildren();
                        }
                    }
                    return true;
                }
                return false;
            }
};


typedef DefaultImageablePrimVisitorT<
    _VisitModels,true>   _RecursiveVisitImageableModels;


struct _VisitNonGroupModels
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
            {
                if(BOOST_UNLIKELY(prim.IsModel() && !prim.IsGroup())) {
                    ctl.PruneChildren();
                    return true;
                }
                return false;
            }
};

typedef DefaultImageablePrimVisitorT<_VisitNonGroupModels>   _VisitImageableModels;


struct _VisitGroups
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
            {
                if(BOOST_UNLIKELY(prim.IsGroup())) {
                    ctl.PruneChildren();
                    return true;
                }
                return false;
            }
};

typedef DefaultImageablePrimVisitorT<_VisitGroups>   _VisitImageableGroups;

struct _VisitLights
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
    {
        UsdLuxLight light(prim);
        if(BOOST_UNLIKELY((bool)light)) {
            ctl.PruneChildren();
            return true;
        }
        return false;
    }
};



typedef DefaultImageablePrimVisitorT<_VisitLights>   _VisitImageableLights;


struct _VisitComponentsAndBoundables
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
            {

                if(BOOST_UNLIKELY(prim.IsA<UsdGeomBoundable>())) {
                    ctl.PruneChildren();
                    return true;
                }                    
                UsdModelAPI model(prim);
                if(BOOST_UNLIKELY((bool)model)) {
                    TfToken kind;
                    model.GetKind(&kind);
                    if( BOOST_UNLIKELY( 
                          KindRegistry::IsA(kind, KindTokens->component) ||
                          KindRegistry::IsA(kind, KindTokens->subcomponent))) {
                        ctl.PruneChildren();
                        return true;
                    }
                }
                return false;
            }
};


typedef DefaultImageablePrimVisitorT<
    _VisitComponentsAndBoundables > _VisitImageableComponentsAndBoundables;


template <class MatchKind>
struct _VisitByKindT
{
    bool    operator()(const UsdPrim& prim,
                       UsdTimeCode time,
                       GusdUSD_TraverseControl& ctl)
            {
                UsdModelAPI model(prim);
                if(BOOST_UNLIKELY((bool)model)) {
                    TfToken kind;
                    model.GetKind(&kind);
                    if(BOOST_UNLIKELY(MatchKind()(kind))) {
                        ctl.PruneChildren();
                        return true;
                    }
                }
                return false;
            }
};

struct _MatchComponents
{
    bool    operator()(const TfToken& kind) const
            {
                return KindRegistry::IsA(kind, KindTokens->component) ||
                       KindRegistry::IsA(kind, KindTokens->subcomponent);
            }
};

struct _MatchAssemblies
{
    bool    operator()(const TfToken& kind) const
            {
                return KindRegistry::IsA(kind, KindTokens->assembly);
            }
};




typedef DefaultImageablePrimVisitorT<
    _VisitByKindT<_MatchComponents> > _VisitImageableComponents;

typedef DefaultImageablePrimVisitorT<
    _VisitByKindT<_MatchAssemblies> >    _VisitImageableAssemblies;


} /*namespace*/


#define _DECLARE_STATIC_TRAVERSAL(name,visitor)         \
    const GusdUSD_Traverse& name()                      \
    {                                                   \
        static visitor v;                               \
        static GusdUSD_TraverseSimpleT<visitor> t(v);   \
        return t;                                       \
    }
    

_DECLARE_STATIC_TRAVERSAL(GetComponentTraversal,            _VisitImageableComponents);
_DECLARE_STATIC_TRAVERSAL(GetComponentAndBoundableTraversal,_VisitImageableComponentsAndBoundables);
_DECLARE_STATIC_TRAVERSAL(GetAssemblyTraversal,             _VisitImageableAssemblies);
_DECLARE_STATIC_TRAVERSAL(GetModelTraversal,                _VisitImageableModels);
_DECLARE_STATIC_TRAVERSAL(GetGroupTraversal,                _VisitImageableGroups);
_DECLARE_STATIC_TRAVERSAL(GetBoundableTraversal,            _VisitImageableBoundables);
_DECLARE_STATIC_TRAVERSAL(GetGprimTraversal,                _VisitImageableGprims);
_DECLARE_STATIC_TRAVERSAL(GetMeshTraversal,                 _VisitImageableMeshes);
_DECLARE_STATIC_TRAVERSAL(GetLightTraversal,                _VisitImageableLights);

_DECLARE_STATIC_TRAVERSAL(GetRecursiveModelTraversal,       _RecursiveVisitImageableModels);

namespace {

GusdUSD_TraverseType stdTypes[] = {
    GusdUSD_TraverseType(&GetComponentAndBoundableTraversal(), "std:components",
                         "Components", NULL,
                         "Returns default-imageable models with a "
                         "component-derived kind."),    
    GusdUSD_TraverseType(&GetGroupTraversal(), "std:groups",
                         "Groups", NULL,
                         "Returns default-imageable groups (of any kind)."),
    GusdUSD_TraverseType(&GetBoundableTraversal(), "std:boundables",
                         "Gprims", NULL,
                         "Return leaf geometry primitives, instances, and procedurals."),
    GusdUSD_TraverseType(&GetLightTraversal(), "std:lights",
                         "Lights", NULL,
                         "Return light primitives."),
};


} /*namespace*/

} /*namespace GusdUSD_StdTraverse */

PXR_NAMESPACE_CLOSE_SCOPE

