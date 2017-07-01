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
#include "visitor.h"
#include "UT_Usd.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <boost/foreach.hpp>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::endl;
using std::string;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

GusdVisitor::GusdVisitor()
    : m_activeState(TRUE_STATE)
    , m_refState(ANY_STATE)
    , m_recurseUnmatched(true)
    , m_recurseChildren(true)
    , m_minDepth(0)
    , m_maxDepth(INT_MAX)
    , m_visitAnyClass(true)
    , m_visitModels(false)
    , m_visitModelGroups(false)
    , m_modelKindPattern()
{
    m_purpose[UsdGeomTokens->default_] = true;
    m_purpose[UsdGeomTokens->guide] = false;
    m_purpose[UsdGeomTokens->render] = false;
    m_purpose[UsdGeomTokens->proxy] = false;
    SetVisitGeometryPrims(false);
    SetVisitPrimType(TfToken("Scope"), false);
    SetVisitPrimType(TfToken("Xform"), false);
    SetVisitPrimType(TfToken("PxPointInstancer"), false);
}

void
GusdVisitor::SetModelKindPattern(const string & pat)
{
    m_modelKindPattern = pat;
}


void
GusdVisitor::SetVisitGeometryPrims(bool state)
{
    SetVisitPrimType(TfToken("BasisCurves"), state);
    SetVisitPrimType(TfToken("Cube"), state);
    SetVisitPrimType(TfToken("Cylinder"), state);
    SetVisitPrimType(TfToken("Mesh"), state);
    SetVisitPrimType(TfToken("NurbsCurves"), state);
    SetVisitPrimType(TfToken("Points"), state);
    SetVisitPrimType(TfToken("Sphere"), state);
}

bool
GusdVisitor::VisitPrims( const UsdPrim& prim,
                           vector<UsdPrim>* results ) const
{
    bool active = true;
    TfToken purpose;
    GusdUT_GetInheritedPrimInfo(prim, active, purpose);
    int depth = 0;
    return recursePrims( prim, active, purpose, depth, results);
}

bool
GusdVisitor::recursePrims(const UsdPrim& prim,
                          bool active,
                          TfToken purpose,
                          int depth,
                          vector<UsdPrim>* results ) const 
{
    DBG( cout << "GusdVisitor::_RecursePrims " << prim.GetPath() << endl );

    // Try to avoid sampling data that will only be ignored
    // due to USD inheritance semantics.
    if(m_activeState != ANY_STATE)
        active = prim.IsActive();
    // Skip these queries on the pseudo-root.
    if(prim.GetPath() != SdfPath::AbsoluteRootPath()) {
        if (purpose == UsdGeomTokens->default_) {
            if(UsdAttribute attr = prim.GetAttribute(UsdGeomTokens->purpose)) {
                attr.Get(&purpose, UsdTimeCode::Default());
            }
        }
    }
    
    // Determine whether to invoke the visit callback on this prim.
    // (Whether we recurse to children is a separate concern;
    // see _recurseUnmatched.)
    bool visit = GetVisitPurpose(purpose);

    if(visit)
    {
        switch(m_activeState)
        {
        case TRUE_STATE: visit = active; break;
        case FALSE_STATE: visit = !active; break;
        default: break;
        };
    }
    if(visit)
    {
        switch(m_refState)
        {
        case TRUE_STATE: visit = prim.HasAuthoredReferences(); break;
        case FALSE_STATE: visit = !prim.HasAuthoredReferences(); break;
        default: break;
        };
    }
    visit = visit && depth <= m_maxDepth && depth >= m_minDepth;
    visit = visit && m_visitPrimType.find(prim.GetTypeName())->second;

    if(visit && (!m_visitAnyClass || !m_modelKindPattern.empty()))
    {
        if(!m_visitAnyClass)
        {
            UsdModelAPI primModel(prim);
            visit = (m_visitModelGroups && primModel.IsGroup()) ||
                (m_visitModels && primModel.IsModel() &&
                 !primModel.IsGroup());
        }
        if(visit && !m_modelKindPattern.empty())
        {
            TfToken modelKind;
            UsdModelAPI(prim).GetKind(&modelKind);
            visit = UT_String(modelKind.GetString()).multiMatch(UT_String(m_modelKindPattern));
        }
    }
    if(!m_recurseUnmatched && !visit)
        return true;
    if(visit)
    {
        results->push_back( prim );
        if(!m_recurseChildren)
            return true;
    }
    // Prune recursion at m_maxDepth.
    if(depth < m_maxDepth)
    {
        BOOST_FOREACH( UsdPrim child, prim.GetChildren())
        {
            if(!recursePrims( child, active,
                              purpose, depth+1, results ))
                return false;
        }
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

