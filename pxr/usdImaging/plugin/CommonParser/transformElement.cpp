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
#include "transformElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
CommonParserTransformElement::CommonParserTransformElement()
    : _pDescription(NULL)
    , _pDeltas(NULL)
    , _oMatrix(_nMatrixElements)
    , _eMatrixComposition(CommonParserTransformParticleTypeNone)
    , _bMatrixSynced(false)
{
    for(int i=0; i<9; i++)
        _nMatrixElements[i] = 0.0f;
}

CommonParserTransformElement::~CommonParserTransformElement()
{
    // Kill the description.
    while(_pDescription) {
        CommonParserTransformParticle* p = _pDescription;
        _pDescription = const_cast<CommonParserTransformParticle*>(p->Next());
        delete(p);
    }

    Reset();
}

void 
CommonParserTransformElement::Reset()
{
    // Only kill the deltas, the full description
    // needs to persist between runs.
    while(_pDeltas) {
        CommonParserTransformParticle* p = _pDeltas;
        _pDeltas = const_cast<CommonParserTransformParticle*>(p->Next());
        delete(p);
    }
}

void 
CommonParserTransformElement::Push(CommonParserTransformElement& /*from*/ oOther)
{
    // Transform needs to be slurped.
    const CommonParserTransformParticle* pTransform = oOther.Description();
    while(pTransform != NULL) {
        AddTransform(*pTransform);
        pTransform = pTransform->Next();
    }
}

// Push from the environment (ie, initialize outermost context)
void 
CommonParserTransformElement::Push(CommonParserEnvironment* pEnv)
{
    // Transform isn't slurped (yet?) ..
}

void
CommonParserTransformElement::Pop(CommonParserTransformElement& /*to*/ oOther)
{
}

CommonParserStatus 
CommonParserTransformElement::AddTransform(const CommonParserTransformParticle& oParticle)
{
    CommonParserStatus eRet;
    eRet = _AddToList(_pDescription,oParticle);
    if(!eRet.Succeeded())
        return eRet;

    eRet = _AddToList(_pDeltas,oParticle);
    if(!eRet.Succeeded())
        return eRet;

    this->_bMatrixSynced = false;

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserTransformElement::RemoveIdenticalTransform(const CommonParserTransformParticle& oParticle)
{
    CommonParserTransformParticle* pGone = _RemoveFromList(_pDescription,oParticle);
    if(pGone != NULL) {
        this->_bMatrixSynced = false;
        delete(pGone);
    }

    return _AddToList(_pDeltas,oParticle);
}

CommonParserStatus 
CommonParserTransformElement::RemoveSameTypeTransform(const CommonParserTransformParticle& oParticle)
{
    CommonParserTransformParticle* pGone = _RemoveFromList(_pDescription,oParticle.Type());
    if(pGone != NULL) {
        this->_bMatrixSynced = false;
        delete(pGone);
    }

    return _AddToList(_pDeltas,oParticle);
}


CommonParserStatus 
CommonParserTransformElement::ReplaceTransform(const CommonParserTransformParticle& oParticle)
{
    if(_ReplaceInList(_pDescription,oParticle) != NULL)
        this->_bMatrixSynced = false;

    return _AddToList(_pDeltas,oParticle);
}

CommonParserTransformParticle* 
CommonParserTransformElement::GetParticle(CommonParserTransformParticleType eType,
                                          CommonParserTransformParticle* pList)
{
    while(pList != NULL) {
        if(pList->Type() == eType)
            return pList;
        else
            pList = const_cast<CommonParserTransformParticle*>(pList->Next());
    }
    return NULL;
}


CommonParserStatus 
CommonParserTransformElement::_AddToList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle)
{
    if(pList != NULL)
        pList->Append(oParticle.Clone());
    else // (or start a new list with a clone.)
        pList = oParticle.Clone();

    return CommonParserStatusTypeOk;
}

CommonParserTransformParticle* 
CommonParserTransformElement::_RemoveFromList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle)
{
    // Empty List, nothing to delete.
    if(pList == NULL)
        return NULL;

    CommonParserTransformParticle* pParticle = pList;
    // Need to remove the head of the list?
    if(*pParticle == oParticle) {
        pList = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        pParticle->SetNext(NULL);
        return pParticle;
    }
    else { // Let's traverse into the list.
        CommonParserTransformParticle* pPrev = pParticle;
        pParticle = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        while(pParticle != NULL) {
            if(*pParticle == oParticle) {
                pPrev->SetNext(const_cast<CommonParserTransformParticle*>(pParticle->Next())); // jump over this particle
                pParticle->SetNext(NULL);          // and unlink it.
                return pParticle;
            }
            // Advance our two pointers.
            pPrev = pParticle;
            pParticle = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        }

        // Not found.  Nothing to remove.  Let's say so.
        return NULL;
    }
}

CommonParserTransformParticle* 
CommonParserTransformElement::_RemoveFromList(CommonParserTransformParticle*& pList, 
                                              const CommonParserTransformParticleType eType)
{
    // Empty List, nothing to delete.
    if(pList == NULL)
        return NULL;

    CommonParserTransformParticle* pParticle = pList;
    // Need to remove the head of the list?
    if(pParticle->Type() == eType) {
        pList = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        pParticle->SetNext(NULL);
        return pParticle;
    }
    else { // Let's traverse into the list.
        CommonParserTransformParticle* pPrev = pParticle;
        pParticle = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        while(pParticle != NULL) {
            if(pParticle->Type() == eType) {
                pPrev->SetNext(const_cast<CommonParserTransformParticle*>(pParticle->Next())); // jump over this particle
                pParticle->SetNext(NULL);          // and unlink it.
                return pParticle;
            }
            // Advance our two pointers.
            pPrev = pParticle;
            pParticle = const_cast<CommonParserTransformParticle*>(pParticle->Next());
        }

        // Not found.  Nothing to remove.  Let's say so.
        return NULL;
    }
}


// Replaces the particle in the list with one that is of the same type as the particle given;
// if one is not found, the particle is added.
CommonParserTransformParticle* 
CommonParserTransformElement::_ReplaceInList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle)
{
    CommonParserTransformParticle* pInList = GetParticle(oParticle.Type(),pList);
    if(pInList != NULL)
        *pInList = oParticle;
    else
        _AddToList(pList,oParticle);

    return pInList;
}


// Builds the matrix represented as the product of all TransformParticles, taken in order.
// This implementation caches the matrix for repeated queries (as the transform may be in
// effect over several runs, each of which would theoretically want to query the matrix.)
CommonParserTransformParticleType 
CommonParserTransformElement::AsMatrix(CommonParserMatrix* pMat)  const
{
    if(!this->_bMatrixSynced) {
        // Defy our "const" status for this, to update the cache.
        CommonParserTransformElement* pThis = const_cast<CommonParserTransformElement*>(this);
        pThis->_eMatrixComposition = CommonParserTransformParticleTypeNone;
        pThis->_oMatrix.SetIdentity();

        const CommonParserTransformParticle* pParticle = _pDescription;
        while(pParticle != NULL) {
            // Save ourselves a bunch of multiplies against 
            // an identity matrix.
            if(!pParticle->IsIdentity()) {
                NUMBER n[9];
                CommonParserMatrix m(n);
                m.SetIdentity();
                    pParticle->SetMatrix(m);

                pThis->_oMatrix *= m;
                pThis->_eMatrixComposition = (CommonParserTransformParticleType)
                    ((int) pThis->_eMatrixComposition | (int)pParticle->Type());
            }

            pParticle = pParticle->Next();
            pThis->_bMatrixSynced = true;
        }
    }
    if(pMat != NULL)
        *pMat = this->_oMatrix;

    return this->_eMatrixComposition;
}

const CommonParserTransformParticle* 
CommonParserTransformElement::Description() const
{
    return this->_pDescription;
}

const CommonParserTransformParticle* 
CommonParserTransformElement::Deltas() const
{
    return this->_pDeltas;
}

PXR_NAMESPACE_CLOSE_SCOPE
