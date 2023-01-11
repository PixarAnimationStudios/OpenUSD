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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STYLE_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STYLE_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
    
/// \class CommonParserStyleDescriptionElement
///
/// This is an implementation of the ATOM CommonParserStyleDescription interface. It's to be 
/// used by a parser in support of the parsing operation.
/// 
class CommonParserStyleDescriptionElement: public CommonParserStyleDescription
{
public:
    /// The default constructor.
    CommonParserStyleDescriptionElement();

    /// List-copy constructor.
    CommonParserStyleDescriptionElement(const CommonParserStyleParticle* pOtherList);

    /// The destructor.
    virtual ~CommonParserStyleDescriptionElement();

    /// Adds to the full Description of the style. Employs "set" semantics, meaning that only one 
    /// particle of any given type resides in the set; Any existing element of the same type is
    /// replaced.
    /// The caller retains ownership of oParticle; the description will contain its own clone.
    CommonParserStatus AddToDescription(const CommonParserStyleParticle& oParticle);
    CommonParserStatus RemoveFromDescription(const CommonParserStyleParticleType eType);

    /// Gets the first particle of the type indicated.
    CommonParserStyleParticle* GetDescriptionParticle(CommonParserStyleParticleType eType) const;

    /// Implement CommonParserStyleDescription::Description().
    const CommonParserStyleParticle* Description() const override;

    /// Implement CommonParserStyleDescription::DescriptionParticle().
    const CommonParserStyleParticle* DescriptionParticle(
        CommonParserStyleParticleType eType) const override;

protected:
    // Utility functions.

    /// Gets the first particle of the type indicated, but from the indicated list.
    static CommonParserStyleParticle* _GetParticle(CommonParserStyleParticleType eType,
                                                   const CommonParserStyleParticle* pList);

    /// Implements the set semantics, ensuring "uniqueness" of particle types in the set.
    static CommonParserStatus _AddToSet(CommonParserStyleParticle*& pSet, 
                                        const CommonParserStyleParticle& oParticle);

    static CommonParserStyleParticle* _RemoveFromList(CommonParserStyleParticle*& pList, 
                                                      const CommonParserStyleParticleType eType);

private:
    CommonParserStyleParticle* _pDescription; // current state
};

/// \class CommonParserStyleChangeElement
///
/// This is an implementation of the ATOM CommonParserStyleChange interface.It's to be 
/// used by a parser in support of the parsing operation.
/// 
class CommonParserStyleChangeElement: public CommonParserStyleDescriptionElement, 
                                      public CommonParserStyleChange
{
public:
    /// The default constructor.
    CommonParserStyleChangeElement();

    /// The destructor.
    ~CommonParserStyleChangeElement();

    /// Adds to the Delta list, (as well as keeping the overall Description in sync.)
    /// Particles are added to the end of the list (that is, the order is preserved)
    /// The delta list does not maintain set semantics, so several (possibly redundant)
    /// particles may co-exist here (but their manifestation in the Description remains
    /// unique, with last-in prevailing.)
    CommonParserStatus AddDelta(const CommonParserStyleParticle& oParticle);

    /// Gets the nth particle of the type indicated, from the Delta list.
    CommonParserStyleParticle* GetDeltaParticle(CommonParserStyleParticleType eType,int n=1);

    /// Reset the change.
    void Reset();

    /// Push from some outer context.
    void Push(CommonParserStyleChangeElement&);

    /// Push from the environment (ie, initialize outermost context).
    void Push(CommonParserEnvironment*);

    /// Pop style changes off, update outermost delta list.
    void Pop(CommonParserStyleChangeElement&);

    /// Implement CommonParserStyleChangeElement::Description().
    const CommonParserStyleParticle* Description() const override;

    /// Implement CommonParserStyleChangeElement::DescriptionParticle().
    const CommonParserStyleParticle* DescriptionParticle(CommonParserStyleParticleType eType) const override;

    /// Implement CommonParserStyleChange::Deltas().
    const CommonParserStyleParticle* Deltas() const override;

private:
    CommonParserStyleParticle* _pDeltas;
};

/// \class CommonParserEmptyStyleTable
///
/// Simple implementation.
/// 
class CommonParserEmptyStyleTable: public CommonParserStyleTable
{
public:
    /// The default constructor.
    CommonParserEmptyStyleTable();

    /// Access the style in the table.
    const CommonParserStyleDescription* operator[] (const CommonParserStRange& sName) const;

    /// Add a style to the table.
    CommonParserStatus AddStyle(const CommonParserStRange& sName, 
                                const CommonParserStyleDescription* pStyle);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STYLE_ELEMENT_H
