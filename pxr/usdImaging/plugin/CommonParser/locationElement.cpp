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

#include "locationElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
CommonParserLocationElement::CommonParserLocationElement()
    : _eSemantics(CommonParserSemanticTypeNormal)
    , _pOperations(NULL)
{
}

CommonParserLocationElement::~CommonParserLocationElement()
{
    Reset();
}

void 
CommonParserLocationElement::Reset()
{
    // Kill the operations.
    while(_pOperations) {
        CommonParserLocationParticle* p = _pOperations;
        _pOperations = const_cast<CommonParserLocationParticle*>(p->Next());
        delete(p);
    }
    this->_eSemantics = CommonParserSemanticTypeNormal;
}

void 
CommonParserLocationElement::Push(CommonParserLocationElement& oOther)
{
    // Location isn't slurped.
    // We assume it's Normal, and that markup will
    // explicitly tell us otherwise.
}

// Push from the environment (ie, initialize outermost context)
void 
CommonParserLocationElement::Push(CommonParserEnvironment* pEnv)
{
    // Location isn't slurped (yet?)
}

void 
CommonParserLocationElement::Pop(CommonParserLocationElement& oOther)
{
}


void 
CommonParserLocationElement::SetSemantics(CommonParserSemanticType eType)
{
    this->_eSemantics = eType;
}

bool 
CommonParserLocationElement::AddOperation(const CommonParserLocationParticle& oParticle)
{
    return AddToList(this->_pOperations,oParticle);
}


bool 
CommonParserLocationElement::AddToList(
    CommonParserLocationParticle*& pList, 
    const CommonParserLocationParticle& oParticle)
{
    CommonParserLocationParticle* pClone = oParticle.Clone();
    if(pClone == NULL)
        return false;

    if(pList != NULL)
        pList->Append(pClone);
    else // (or start a new list with a clone.)
        pList = pClone;

    return false;
}



// Describes the nature of the location change.
CommonParserSemanticType 
CommonParserLocationElement::Semantics() const
{
    return this->_eSemantics;
}

// Zero or more operations to effect the location change.
CommonParserLocationParticle* 
CommonParserLocationElement::Operations() const
{
    return this->_pOperations;
}

PXR_NAMESPACE_CLOSE_SCOPE
