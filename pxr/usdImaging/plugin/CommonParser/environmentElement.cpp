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
#include "environmentElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
// Implement a dummy resolver, since most of the time,
// parsers won't need one.
CommonParserStatus 
PassthroughReferenceResolverElement::Initialize()
{
    return CommonParserStatusTypeOk;
}

// Requests the resolver to resolve a reference.
CommonParserStatus 
PassthroughReferenceResolverElement::Resolve(
    const CommonParserStRange sParserName,
    const CommonParserStRange sReference,
    CommonParserStRange& sResult,
    CommonParserEnvironment* pEnv)
{
    sResult = sReference;
    return CommonParserStatusTypeUnchanged;
}

// Allows the resolver to clean up.
CommonParserStatus 
PassthroughReferenceResolverElement::Terminate()
{
    return CommonParserStatusTypeOk;
}

// This is just used for defaulting the environment.
PassthroughReferenceResolverElement  gDummyResolver;

CommonParserEnvironmentElement::CommonParserEnvironmentElement(
    CommonParserSink* pSink,
    CommonParserStyleTable* pStyleTable,
    CommonParserColor rgbaCanvas)
    : _pSink(pSink)
    , _pStyleTable(pStyleTable)
    , _pResolver(&gDummyResolver)
    , _rgbaCanvas(rgbaCanvas)
{
    assert(_pSink);
    assert(_pStyleTable);
}

CommonParserStatus 
CommonParserEnvironmentElement::SetSink(CommonParserSink* pSink)
{
    // if this->_pSink->SinkState() == keWaiting && pSink->SinkState() == keWaiting ...
    _pSink = pSink;
    return CommonParserStatusTypeOk;
}


CommonParserStatus
CommonParserEnvironmentElement::SetResolver(
    CommonParserReferenceResolver* pResolver)
{
    _pResolver = pResolver? pResolver : &gDummyResolver;
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserEnvironmentElement::UpdateAmbientStyle(
    const CommonParserStyleParticle& oParticle)
{
    return _AmbientStyle.AddToDescription(oParticle);
}

CommonParserStatus
CommonParserEnvironmentElement::UpdateAmbientTransform(
    const CommonParserTransformParticle& oParticle)
{
    return _AmbientTransform.AddTransform(oParticle);
}

const CommonParserStyleDescription*
CommonParserEnvironmentElement::AmbientStyle() const
{
    return &_AmbientStyle;
}
const CommonParserTransform*
CommonParserEnvironmentElement::AmbientTransform() const
{
    return &_AmbientTransform;
}

const CommonParserStyleTable* 
CommonParserEnvironmentElement::StyleDictionary() const
{
    return _pStyleTable;
}


CommonParserSink*
CommonParserEnvironmentElement::Sink() const
{
    return _pSink;
}


CommonParserReferenceResolver*
CommonParserEnvironmentElement::References() const
{
    return _pResolver;
}

CommonParserColor
CommonParserEnvironmentElement::CanvasColor() const
{
    return _rgbaCanvas;
}
PXR_NAMESPACE_CLOSE_SCOPE