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
#ifndef __GUSD_VISITOR_H__
#define __GUSD_VISITOR_H__

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"

#include <UT/UT_Interrupt.h>

PXR_NAMESPACE_OPEN_SCOPE

class GU_Detail;


/**
   Class for performing controlled recursion through usd prims.
   This includes the ability to exclude/include by visibility state,
   purpose, prim type, etc.
   This is the base algorithm that the import style used in
   pxh_usdRefsSOP is based on.
*/
class GusdVisitor
{
public:
    typedef enum
    {
        TRUE_STATE,
        FALSE_STATE,
        ANY_STATE,
    } TriState;

    GusdVisitor();
    
    bool        VisitPrims(
                    const UsdPrim& prim,
                    std::vector<UsdPrim>* results ) const;

    TriState    GetActiveState() const          { return m_activeState; }
    void        SetActiveState(TriState state)  { m_activeState = state; }

    TriState    GetRefState() const             { return m_refState; }
    void        SetRefState(TriState state)     { m_refState = state; }

    inline bool GetVisitPurpose(const TfToken& purpose) const;
    void        SetVisitPurpose(const TfToken& purpose, bool state)
                { m_purpose[purpose] = state; }

    bool        GetRecurseUnmatched() const     { return m_recurseUnmatched; }
    void        SetRecurseUnmatched(bool state) { m_recurseUnmatched = state; }

    bool        GetRecurseChildren() const      { return m_recurseChildren; }
    void        SetRecurseChildren(bool state)  { m_recurseChildren = state; }

    int         GetMinDepth() const             { return m_minDepth; }
    void        SetMinDepth(int depth)          { m_minDepth = depth; }

    int         GetMaxDepth() const             { return m_maxDepth; }
    void        SetMaxDepth(int depth)
                { m_maxDepth = depth < 0 ? INT_MAX : depth; }

    inline bool VisitPrimType(const TfToken& typeName) const;
    void        SetVisitPrimType(const TfToken& typeName, bool state)
                { m_visitPrimType[typeName] = state; }

    bool        GetVisitAnyClass() const        { return m_visitAnyClass; }
    void        SetVisitAnyClass(bool state)    { m_visitAnyClass = state; }

    bool        GetVisitModels() const          { return m_visitModels; }
    void        SetVisitModels(bool state)      { m_visitModels = state; }

    bool        GetVisitModelGroups() const     { return m_visitModelGroups; }
    void        SetVisitModelGroups(bool state) { m_visitModelGroups = state; }


    const std::string& GetModelKindPattern() const     { return m_modelKindPattern; }
    void        SetModelKindPattern(const std::string &pat );

    /** Toggle visiting of geometry-containing prims.
        Just a convenience method that calls SetVisitPrimType() for each
        geometry-holding prim type.*/
    void        SetVisitGeometryPrims(bool state);

protected:

    bool        recursePrims(
                    const UsdPrim& prim,
                    bool active, 
                    TfToken purpose, 
                    int depth,
                    std::vector<UsdPrim>* results ) const;
                       
protected:
    TriState                m_activeState;
    TriState                m_refState;
    std::map<TfToken,bool>  m_purpose;

    bool                    m_recurseUnmatched;
    bool                    m_recurseChildren;
    int                     m_minDepth;
    int                     m_maxDepth;

    std::map<TfToken,bool>  m_visitPrimType;
    bool                    m_visitAnyClass;
    bool                    m_visitModels;
    bool                    m_visitModelGroups;
    std::string             m_modelKindPattern;
};


bool
GusdVisitor::GetVisitPurpose(const TfToken& purpose) const
{
    std::map<TfToken,bool>::const_iterator it = m_purpose.find(purpose);
    return it == m_purpose.end() ? false : it->second;
}


bool
GusdVisitor::VisitPrimType(const TfToken& typeName) const
{
    std::map<TfToken,bool>::const_iterator it = m_visitPrimType.find(typeName);
    return it == m_visitPrimType.end() ? false : it->second;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* __GUSD_VISITOR_H__ */
