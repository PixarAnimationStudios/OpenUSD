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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_LOCATION_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_LOCATION_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserLocationElement
///
/// The location implementation in CommonParser module
///
class CommonParserLocationElement: public CommonParserLocation
{
public:
    /// The default constructor.
    CommonParserLocationElement();

    /// The destructor.
    ~CommonParserLocationElement();

    /// Sets one (or a bunch) of semantics at once.
    void SetSemantics(CommonParserSemanticType eType);

    /// Adds one semantic at a time.
    void AddSemantic(CommonParserSemanticType eType)
    {
        _eSemantics = (CommonParserSemanticType)((_eSemantics & ~CommonParserSemanticTypeNormal)|eType);
    }

    /// Add a location operator.
    bool AddOperation(const CommonParserLocationParticle& oParticle);

    /// Reset the location.
    void Reset();

    /// Push from some outer location context.
    void Push(CommonParserLocationElement&);

    /// Push from the environment (ie, initialize outermost context)
    void Push(CommonParserEnvironment*);

    /// Pop the last pushed element.
    void Pop(CommonParserLocationElement&);

    /// Describes the nature of the location change.
    CommonParserSemanticType Semantics() const override;

    /// Zero or more operations to effect the location change.
    CommonParserLocationParticle* Operations() const override;

protected:
    bool AddToList(CommonParserLocationParticle*& pList, 
                   const CommonParserLocationParticle& oParticle);
private:
    // What does this location description mean?
    CommonParserSemanticType _eSemantics;

    // What are the operations that go into making this location change.
    CommonParserLocationParticle* _pOperations;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_LOCATION_ELEMENT_H
