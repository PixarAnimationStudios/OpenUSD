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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_TEXT_RUN_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_TEXT_RUN_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"
#include "locationElement.h"
#include "structureElement.h"
#include "styleElement.h"
#include "transformElement.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserTextRunElement
///
/// The textRun implementation in CommonParser module
///
class CommonParserTextRunElement: public CommonParserTextRun
{
public:
    /// Standard constructor, ignoring environment.
    CommonParserTextRunElement()
        : _iDepth(0)
        , _pParent(NULL)
        , _OwnText(false)
    {
        // Nothing to Push; this *is* the outermost context...
    }

    /// Standard constructor, accessing evironment for initial settings...
    /// Use this for synchronizing with the ambient style in the environment.
    /// (Alternative: use default ctor above andInitFrom(pEnv) below.)
    CommonParserTextRunElement(CommonParserEnvironment* pEnv)
        : _iDepth(0)
        , _pParent(NULL)
        , _OwnText(false)    
    {
        _Push(pEnv);
    }

    /// Recursion constructor: makes a snapshot of the current (parent) state.  Parser should
    /// resort to using this constructor when recursive markup occurs (such as RTF's {...} or SVG's
    /// nested elements.
    /// (Alternative: use default ctor above and InitFrom(pParent) below.)
    CommonParserTextRunElement(CommonParserTextRunElement* pParent)
        : _iDepth(0)
        , _pParent(NULL)
        , _OwnText(false)    
    {
        InitFrom(pParent);
    }

    /// Clean up.
    ~CommonParserTextRunElement()
    {
        _Pop();
    }

    // Needed if you use the default constructor and you're processing a nested context of a 
    /// nested markup language.
    void InitFrom(CommonParserTextRunElement* pParent)
    {
        _pParent = pParent;
        _oStructure.SetOuter(&(_pParent->_oStructure));
        //_iDepth = _pParent->_iDepth+1;

        _Push();
    }

    /// Needed if you use the default constructor and you're processing the outermost context.
    void InitFrom(CommonParserEnvironment* pEnv)
    {
        _pParent = NULL;
        _iDepth = 0;

        _Push(pEnv);
    }

    /// Writable (non-const) access to Location
    CommonParserLocationElement& Location()
    {
        return _oLocation;
    }

    /// Writable (non-const) access to Contents
    CommonParserStRange& Contents()
    {
        return _oContents;
    }

    /// Writable (non-const) access to Style
    CommonParserStyleChangeElement& Style()
    {
        return _oStyle;
    }

    /// Writable (non-const) access to Transform
    CommonParserTransformElement& Transform()
    {
        return _oTransform;
    }

    /// True if the text in _oContents is owned by the element.
    /// False if it is just a reference.
    bool OwnText()
    {
        return _OwnText;
    }

    /// Set if the text in _oContents is owned by the element.
    void OwnText(bool value)
    {
        _OwnText = value;
    }

    /// Cleanup needed between TextRun notifications.
    /// Parser should call this after the notification is complete in order to be ready
    ///  for the next text run.
    void Reset()
    {
        _oLocation.Reset();
        _oContents.Reset();

        _oStyle.Reset();
        _oTransform.Reset();
        _OwnText = false;
    }

    /// Test if the textrun is reset.
    bool IsReset()
    {
        return _oContents.Length() == 0;
    }

    /// Implementations of the CommonParserTextRun interface
    /// Implement CommonParserTextRun::Structure().
    const CommonParserStructure* Structure() const override
    {
        return &_oStructure;
    }

    /// Implement CommonParserTextRun::Style().
    const CommonParserStyleChange* Style() const override
    {
        return &_oStyle;
    }

    /// Implement CommonParserTextRun::Transform().
    const CommonParserTransformChange* Transform() const override
    {
        return &_oTransform;
    }

    /// Implement CommonParserTextRun::Contents().
    const CommonParserStRange Contents() const override
    {
        return _oContents;
    }

    /// Implement CommonParserTextRun::Location().
    const CommonParserLocation* Location() const override
    {
        return &_oLocation;
    }
private:
    /// Push from nested context.
    void _Push(); 

    /// Push from outermost context (accessing Env's AmbientStyle).
    void _Push(CommonParserEnvironment*);

    /// Pop the last pushed element.
    void _Pop();

    CommonParserStructureElement    _oStructure;
    CommonParserStyleChangeElement  _oStyle;
    CommonParserTransformElement    _oTransform;
    CommonParserLocationElement     _oLocation;
    CommonParserStRange       _oContents;

    int                 _iDepth;

    CommonParserTextRunElement*     _pParent;

    // True if the text in _oContents is owned by the element.
    // False if it is just a reference.
    bool _OwnText;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_STRUCTURE_ELEMENT_H
