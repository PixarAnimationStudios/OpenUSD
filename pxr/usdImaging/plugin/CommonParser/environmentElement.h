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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"
#include "styleElement.h"
#include "transformElement.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserAbandonment
///
/// This is an implementation of the ATOM CommonParserEnvironment interface. It's to be 
/// used by a parser in support of the parsing operation.
/// 
class CommonParserEnvironmentElement: public CommonParserEnvironment 
{
public:
    /// The constructor from the sink, the style table and the color.
    CommonParserEnvironmentElement(CommonParserSink* pSink,
                                   CommonParserStyleTable* pStyleTable,
                                   CommonParserColor rgbaCanvas);

    /// Set the recipient of a parser's effort.
    CommonParserStatus SetSink(CommonParserSink* pSink);

    /// Set the resolver of reference.
    CommonParserStatus SetResolver(CommonParserReferenceResolver*);

    /// Set the ambient style.
    CommonParserStatus UpdateAmbientStyle(const CommonParserStyleParticle& oParticle);

    /// Set the ambient transform.
    CommonParserStatus UpdateAmbientTransform(const CommonParserTransformParticle& oParticle);

    /// Implement CommonParserEnvironmentElement::AmbientStyle().
    const CommonParserStyleDescription*  AmbientStyle() const override;
    /// Implement CommonParserEnvironmentElement::AmbientTransform().
    const CommonParserTransform* AmbientTransform() const override;
    /// Implement CommonParserEnvironmentElement::StyleDictionary().
    const CommonParserStyleTable* StyleDictionary() const override;
    /// Implement CommonParserEnvironmentElement::Sink().
    CommonParserSink* Sink() const override;
    /// Implement CommonParserEnvironmentElement::References().
    CommonParserReferenceResolver* References() const override;
    /// Implement CommonParserEnvironmentElement::CanvasColor().
    CommonParserColor CanvasColor() const override;

private:
    CommonParserStyleDescriptionElement          _AmbientStyle;
    CommonParserTransformElement     _AmbientTransform;
    CommonParserStyleTable*          _pStyleTable;
    CommonParserSink*                _pSink;
    CommonParserReferenceResolver*   _pResolver;
    CommonParserColor                _rgbaCanvas;
};


/// \class PassthroughReferenceResolverElement
///
/// A dummy resolver, just gives back what you passed in.
/// 
class PassthroughReferenceResolverElement: public CommonParserReferenceResolver
{
public:
    CommonParserStatus Initialize();

    // Requests the resolver to resolve a reference.
    CommonParserStatus Resolve(const CommonParserStRange sParserName,
                         const CommonParserStRange sReference,
                               CommonParserStRange& sResult,
                               CommonParserEnvironment* pEnv);

    // Allows the resolver to clean up.
    CommonParserStatus Terminate();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_ELEMENT_H
