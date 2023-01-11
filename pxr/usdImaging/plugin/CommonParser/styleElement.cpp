//
// Copyright 2023 Pixar
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

#include "styleElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
CommonParserStyleDescriptionElement::CommonParserStyleDescriptionElement()
    : _pDescription(0)
{}

// List-copy constructor
CommonParserStyleDescriptionElement::CommonParserStyleDescriptionElement(
    const CommonParserStyleParticle* pOtherList)
    : _pDescription(0)
{
    const CommonParserStyleParticle* pStyle = pOtherList;
    while(pStyle != NULL) {
        AddToDescription(*pStyle);
        pStyle = pStyle->Next();
    }
}

CommonParserStyleDescriptionElement::~CommonParserStyleDescriptionElement()
{
    // Kill the description.
    while(_pDescription) {
        CommonParserStyleParticle* p = _pDescription;
        _pDescription = const_cast<CommonParserStyleParticle*>(p->Next());
        delete(p);
    }
}

const CommonParserStyleParticle* 
CommonParserStyleDescriptionElement::Description() const
{
    return _pDescription;
}

const CommonParserStyleParticle* 
CommonParserStyleDescriptionElement::DescriptionParticle(CommonParserStyleParticleType eType) const
{
    return GetDescriptionParticle(eType);
}

CommonParserStatus 
CommonParserStyleDescriptionElement::AddToDescription(const CommonParserStyleParticle& oParticle)
{
    return _AddToSet(_pDescription,oParticle);
}

CommonParserStatus 
CommonParserStyleDescriptionElement::RemoveFromDescription(const CommonParserStyleParticleType eType)
{
    CommonParserStyleParticle* pGone = _RemoveFromList(_pDescription,eType);
    if(pGone != NULL) {
        delete(pGone);
        return CommonParserStatusTypeOk;
    }

    // Nothing to remove?  Say so.
    return CommonParserStatusTypeUnchanged;
}

// Gets the first particle of the type indicated.
CommonParserStyleParticle* 
CommonParserStyleDescriptionElement::GetDescriptionParticle(CommonParserStyleParticleType eType) const
{
    return _GetParticle(eType,this->_pDescription);
}

/* Style CommonParserParticle List Tools */

// Finds (the first) particle in a list (or set)
// and returns a pointer to it, or NULL if none found.
CommonParserStyleParticle* 
CommonParserStyleDescriptionElement::_GetParticle(
    CommonParserStyleParticleType eType,
    const CommonParserStyleParticle* pList)
{
    while(pList != NULL) {
        if(pList->Type() == eType)
            return const_cast<CommonParserStyleParticle*>(pList);
        else
            pList = const_cast<CommonParserStyleParticle*>(pList->Next());
    }
    return NULL;
}

// Imparts "set" semantics to a list of Particles
// Only one particle of any given type can exist
// within such a set.  Adding a duplicate replaces
// the original with the newer particle's value.
// In all cases, oParticle's ownership remains outside
// of this list (that is, the list contains only copies.)
CommonParserStatus 
CommonParserStyleDescriptionElement::_AddToSet(
    CommonParserStyleParticle*& pSet, 
    const CommonParserStyleParticle& oParticle)
{
    CommonParserStyleParticle* pInList = _GetParticle(oParticle.Type(),pSet);

    // if it's already in the list/set, we copy the
    // contents of the particle over the corresponding
    // value already in the list.
    if(pInList) {
        *pInList = oParticle; 
        return CommonParserStatusTypeReplaced;
    }
    else { // Otherwise, we append a clone to the list
        if(pSet != NULL)
            pSet->Append(oParticle.Clone());
        else // (or start a new list with a clone.)
            pSet = oParticle.Clone();
        return CommonParserStatusTypeOk;
    }
}

CommonParserStyleParticle* 
CommonParserStyleDescriptionElement::_RemoveFromList(
    CommonParserStyleParticle*& pList, 
    const CommonParserStyleParticleType eType)
{
    // Empty List, nothing to delete.
    if(pList == NULL)
        return NULL;

    CommonParserStyleParticle* pParticle = pList;
    // Need to remove the head of the list?
    if(pParticle->Type() == eType) {
        pList = const_cast<CommonParserStyleParticle*>(pParticle->Next());
        pParticle->SetNext(NULL);
        return pParticle;
    }
    else { // Let's traverse into the list.
        CommonParserStyleParticle* pPrev = pParticle;
        pParticle = const_cast<CommonParserStyleParticle*>(pParticle->Next());
        while(pParticle != NULL) {
            if(pParticle->Type() == eType) {
                pPrev->SetNext(const_cast<CommonParserStyleParticle*>(pParticle->Next())); // jump over this particle
                pParticle->SetNext(NULL);          // and unlink it.
                return pParticle;
            }
            // Advance our two pointers.
            pPrev = pParticle;
            pParticle = const_cast<CommonParserStyleParticle*>(pParticle->Next());
        }

        // Not found.  Nothing to remove.  Let's say so.
        return NULL;
    }
}

CommonParserStyleChangeElement::CommonParserStyleChangeElement()
    : _pDeltas(0)
{
}

CommonParserStyleChangeElement::~CommonParserStyleChangeElement()
{
    Reset();
}

void 
CommonParserStyleChangeElement::Reset()
{
    // Kill the deltas.
    while(_pDeltas) {
        CommonParserStyleParticle* p = _pDeltas;
        _pDeltas = const_cast<CommonParserStyleParticle*>(p->Next());
        delete(p);
    }
}

void 
CommonParserStyleChangeElement::Push(CommonParserStyleChangeElement& /*from*/ oOther)
{
    // Is there a pending notification?
    // Did somebody forget to send out an CommonParserTextRun notification
    // before entering the nested context?
    assert(oOther.Deltas() == NULL);

    const CommonParserStyleParticle* pStyle = oOther.Description();
    while(pStyle != NULL) {
        AddToDescription(*pStyle);
        pStyle = pStyle->Next();
    }
}

// Push from the environment (ie, initialize outermost context)
void 
CommonParserStyleChangeElement::Push(CommonParserEnvironment* pEnv)
{
    const CommonParserStyleParticle* pStyle = pEnv->AmbientStyle()->Description();
    while(pStyle != NULL) {
        AddToDescription(*pStyle);
        pStyle = pStyle->Next();
    }
}

void 
CommonParserStyleChangeElement::Pop(CommonParserStyleChangeElement& /*to*/ oOuter)
{
    // Is there a pending notification?
    // Did somebody forget to send out an CommonParserTextRun notification
    // before leaving the nested context?

    // TODO: evaluate this.  It's possible to emerge from two
    // consecutive contexts, the inner one pushing some deltas
    // into the list, and the outer running afoul of this check.
    //assert(Deltas() == NULL);

    const CommonParserStyleParticle* pStyle = Description();

    while(pStyle != NULL) {
        const CommonParserStyleParticle* pOuterStyle = oOuter.GetDescriptionParticle(pStyle->Type());
        // If the outer style contains this particle, but we've changed it
        // let's assert the delta back to that outer value.
        //
        // This doesn't detect the condition where we have 
        // particle X but the outer context doesn't (it was first used
        // in our context.)  The outer Description() will still be
        // correct, but the Delta() just won't pick it up.  It's a matter
        // of the environment containing an AmbientStyle that is complete,
        // such that X always exists.  The problem is, we just don't know
        // what value (some default) to assign to that outer particle.
        if(pOuterStyle != NULL && !(*pStyle == *pOuterStyle)) {
            oOuter.AddDelta(*pOuterStyle);
        }

        pStyle = pStyle->Next();
    }
}

const CommonParserStyleParticle* 
CommonParserStyleChangeElement::Deltas() const
{
    return _pDeltas;
}

// TODO: see if there's something that can be done to consolidate this imp.
// (maybe inline?)
const CommonParserStyleParticle* 
CommonParserStyleChangeElement::Description() const
{
    return CommonParserStyleDescriptionElement::Description();
}

const CommonParserStyleParticle* 
CommonParserStyleChangeElement::DescriptionParticle(CommonParserStyleParticleType eType) const
{
    return CommonParserStyleDescriptionElement::DescriptionParticle(eType);
}

CommonParserStatus 
CommonParserStyleChangeElement::AddDelta(const CommonParserStyleParticle& oParticle)
{
    if(_pDeltas == NULL)
        _pDeltas = oParticle.Clone();
    else
        _pDeltas->Append(oParticle.Clone());

    return AddToDescription(oParticle);
}

// Gets the nth particle of the type indicated, from the Delta list.
CommonParserStyleParticle* 
CommonParserStyleChangeElement::GetDeltaParticle(CommonParserStyleParticleType eType,int n)
{
    CommonParserStyleParticle* pRet = this->_pDeltas;
    do {
        pRet = CommonParserStyleChangeElement::_GetParticle(eType,pRet);
        // Are we done?
        if(--n <= 0)
            break;

        // Can we continue?
        if(pRet)
            pRet = const_cast<CommonParserStyleParticle*>(pRet->Next());
    }while(true);

    return pRet;
}

CommonParserEmptyStyleTable::CommonParserEmptyStyleTable()
{
}

const CommonParserStyleDescription* 
CommonParserEmptyStyleTable::operator[] (const CommonParserStRange& sName) const
{
    // TO DO: implement the addition of styles
    return NULL;
}

CommonParserStatus
CommonParserEmptyStyleTable::AddStyle(
    const CommonParserStRange& sName, 
    const CommonParserStyleDescription* pStyle)
{
    // TO DO: implement the addition of styles.
    return CommonParserStatusTypeNotImplemented;
}
PXR_NAMESPACE_CLOSE_SCOPE
