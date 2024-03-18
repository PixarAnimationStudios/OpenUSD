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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STRUCTURE_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STRUCTURE_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserStructureElement
///
/// The structure implementation in CommonParser module
///
class CommonParserStructureElement: public CommonParserStructure
{
public:
    /// The default constructor.
    CommonParserStructureElement();

    /// Set the outer structure.
    void SetOuter(CommonParserStructureElement*);

    /// Set the shape of the run.
    void SetShape(CommonParserShapeType eShape);

    /// Set if the structure is continuous with previous one.
    void SetContinuous(bool);

    /// Current depth within the markup.
    int Depth() const override
    {
        return _iDepth;
    }

    /// Pointer to an outer CommonParserStructure (with Depth()-1)
    CommonParserStructure* Outer() const override
    {
        return _pOuter;
    }

    /// What is the "shape" of the run?  Does it flow and wrap, or... ?
    CommonParserShapeType Shape() const override
    {
        return _eShape;
    }

    /// Is selection considered continuous with previous run?
    bool Continuous() const override
    {
        return _bContinuous;
    }

private:
    CommonParserStructureElement* _pOuter;
    int               _iDepth;
    CommonParserShapeType _eShape;
    bool              _bContinuous;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STRUCTURE_ELEMENT_H
