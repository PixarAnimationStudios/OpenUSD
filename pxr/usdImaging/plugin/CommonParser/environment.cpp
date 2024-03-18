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

#include "environment.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonParserMarkupEnvironment::CommonParserMarkupEnvironment(
    CommonParserSink* pSink, 
    CommonParserStyleTable* pStyleTable, 
    CommonParserColor rgbaCanvas,
    CommonParserAmbient* ambient, 
    CommonParserReferenceResolver* resolver)
    : _styleTable(pStyleTable)
    , _sink(pSink)
    , _resolver(resolver)
    , _rgbaCanvas(rgbaCanvas)
    , _textAmbient(ambient)
    , _consumeState(CommonParserRunStatus::CommonParserRunStatusNotSet)
{
}

const CommonParserStyleDescription* 
CommonParserMarkupEnvironment::AmbientStyle() const
{
    return &_ambientStyle;
}
const CommonParserTransform* 
CommonParserMarkupEnvironment::AmbientTransform() const
{
    return &_ambientTransform;
}

const CommonParserStyleTable* 
CommonParserMarkupEnvironment::StyleDictionary() const
{
    return _styleTable;
}

CommonParserSink* 
CommonParserMarkupEnvironment::Sink() const
{
    return _sink;
}

CommonParserReferenceResolver* 
CommonParserMarkupEnvironment::References() const
{
    return _resolver;
}

CommonParserColor 
CommonParserMarkupEnvironment::CanvasColor() const
{
    return _rgbaCanvas;
}

CommonParserStatus 
CommonParserMarkupEnvironment::SetTextAmbient(CommonParserAmbient* value)
{
    _textAmbient = value;
    return CommonParserStatusTypeOk;
}

CommonParserAmbient* 
CommonParserMarkupEnvironment::GetTextAmbient() const
{
    return _textAmbient;
}

CommonParserRunStatus 
CommonParserMarkupEnvironment::ConsumeState() const
{
    return _consumeState;
}

void 
CommonParserMarkupEnvironment::ConsumeState(CommonParserRunStatus state)
{
    _consumeState = state;
}
PXR_NAMESPACE_CLOSE_SCOPE
