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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_TRANSFORM_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_TRANSFORM_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserTransformElement
///
/// The transform implementation in CommonParser module
///
class CommonParserTransformElement: public CommonParserTransformChange
{
public:
    /// The default constructor.
    CommonParserTransformElement();

    /// The destructor.
    virtual ~CommonParserTransformElement();

    /// Indiscriminately adds a transform to the (end of the) list
    CommonParserStatus AddTransform(const CommonParserTransformParticle& oParticle);

    /// Removes first found transform exactly == to the one given.  Adds oParticle into delta list
    CommonParserStatus RemoveIdenticalTransform(const CommonParserTransformParticle& oParticle);
    
    /// Removes first-found transform with same type. Adds oParticle into delta list.
    CommonParserStatus RemoveSameTypeTransform(const CommonParserTransformParticle& oParticle);

    /// Replaces first found transform of the type of the given particle with the particle.
    /// (or adds it to the list if no match exists yet.)
    /// Adds oParticle into delta list.
    CommonParserStatus ReplaceTransform(const CommonParserTransformParticle& oParticle);
  
    /// Reset the transform.
    void Reset();

    /// Push from some outer context.
    void Push(CommonParserTransformElement&);

    /// Push from the environment.
    void Push(CommonParserEnvironment*);

    /// Pop transform changes off, update outermost delta list.
    void Pop(CommonParserTransformElement&);

    /// Implementing the CommonParserTransform interface
    ///

    /// Gets the aggregate transformation matrix.
    /// describes only the sum of these TransformationElements
    CommonParserTransformParticleType AsMatrix(CommonParserMatrix*) const override;

    /// Gets the list of constituent transformations that go into the matrix.
    const CommonParserTransformParticle* Description() const override;

    /// Gets the list of changes that that were reported.
    const CommonParserTransformParticle* Deltas() const override;

protected:
    ///
    /// Utility functions
    ///

    /// Gets a particle in the list that has the type indicated.
    static CommonParserTransformParticle* GetParticle(
        CommonParserTransformParticleType eType,
        CommonParserTransformParticle* pList);

    ///  Adds the particle to the list.
    static CommonParserStatus _AddToList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle);

    /// Removes the particle from the list that is equal to the particle given.
    static CommonParserTransformParticle* _RemoveFromList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle);

    /// Removes the particle from the list that has the same type as given.
    static CommonParserTransformParticle* _RemoveFromList(CommonParserTransformParticle*& pList, const CommonParserTransformParticleType eType);

    /// Replaces the particle in the list with one that is of the same type as the particle given;
    /// if one is not found, the particle is added.
    static CommonParserTransformParticle* _ReplaceInList(CommonParserTransformParticle*& pList, const CommonParserTransformParticle& oParticle);

private:
    CommonParserTransformParticle* _pDescription;
    CommonParserTransformParticle* _pDeltas;

    // Caching support for the CommonParserMatrix method.
    NUMBER _nMatrixElements[9];
    CommonParserMatrix _oMatrix;
    CommonParserTransformParticleType _eMatrixComposition;
    bool _bMatrixSynced;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_TRANSFORM_ELEMENT_H
