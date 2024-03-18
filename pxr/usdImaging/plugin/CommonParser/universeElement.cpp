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

#include "universeElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
// Q: What do you name the instance of our (ATOM) Universe?
CommonParserUniverseElement* FortyTwo = NULL;
CommonParserUniverseWrapper FortyTwoWrapper; // Should be obvious, no? ;-)

CommonParserUniverse* BigBang()
{
    if(FortyTwo == NULL)
        FortyTwo = new CommonParserUniverseElement;
    return FortyTwo;
}


CommonParserUniverseWrapper::CommonParserUniverseWrapper()
{
}
CommonParserUniverseWrapper::~CommonParserUniverseWrapper()
{
    if(FortyTwo != NULL)
        delete FortyTwo;
    FortyTwo = NULL;
}

CommonParserUniverseElement::CommonParserUniverseElement()
: _iCount(0)
{
    for(int i=0; i<MAX_PARSERS_IN_UNIVERSE; i++)
        this->_pRegistrants[i] = NULL;
}

CommonParserUniverseElement::~CommonParserUniverseElement()
{
    for(int i = 0; i < _iCount; ++i)
    {
        _pRegistrants[i]->RegisterNull();
    }
}

CommonParserGenerator** 
CommonParserUniverseElement::_Find(const CommonParserStRange& sName)
{
    for(int i=0; i<MAX_PARSERS_IN_UNIVERSE; i++) {
        if(_pRegistrants[i] && _pRegistrants[i]->Name() == sName)
            return &_pRegistrants[i];
    }

    return NULL;
}

CommonParserGenerator** 
CommonParserUniverseElement::_FindEmpty()
{
    for(int i=0; i<MAX_PARSERS_IN_UNIVERSE; i++) {
        if(_pRegistrants[i] == NULL)
            return &_pRegistrants[i];
    }

    return NULL;
}


// Registers a Generator, used by the parsing module
// when introduced to the universe.
CommonParserStatus 
CommonParserUniverseElement::Register(CommonParserGenerator* pNew)
{
    const CommonParserStRange sName = pNew->Name();
    if(_Find(sName))
        return CommonParserStatusTypeAlreadyPresent;

    // TODO: This strategy is now bogus!
    CommonParserGenerator** pp = _FindEmpty();
    if(!pp)
        return CommonParserStatusTypeNoResource;
    
    *pp = pNew;
    _iCount++;

    return CommonParserStatusTypeOk;
}

// Unregisters a Generator.
CommonParserStatus
CommonParserUniverseElement::Unregister(CommonParserGenerator* pDead)
{
    const CommonParserStRange sName = pDead->Name();
    CommonParserGenerator** pp = _Find(sName);
    if(!pp)
        return CommonParserStatusTypeNotPresent;

    *pp = NULL;
    _iCount--;
    // TODO: finish this: move the deleted items down.

    return CommonParserStatusTypeOk;
}

// How many parser/generators are registered?
int 
CommonParserUniverseElement::RegisteredCount()
{
    return _iCount;
}

// Gets a parser generator (by position in registration list)
// to allow the application to begin a parsing operation.
// 0 <= iIndex < RegisteredCount();
CommonParserGenerator*
CommonParserUniverseElement::GetGenerator(int iIndex)
{
    if(iIndex < 0 || iIndex >= _iCount)
        return NULL;

    return this->_pRegistrants[iIndex];
}

// Same as above, but indexed off the CommonParserGenerator::Name()
// method.
CommonParserGenerator*
CommonParserUniverseElement::GetGenerator(const CommonParserStRange& sName)
{
    CommonParserGenerator** pp = _Find(sName);
    if(pp)
        return *pp;
    else
        return NULL;
}
PXR_NAMESPACE_CLOSE_SCOPE
