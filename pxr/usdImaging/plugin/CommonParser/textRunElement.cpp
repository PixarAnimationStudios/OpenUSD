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

#include "textRunElement.h"
#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE
void 
CommonParserTextRunElement::_Push()
{
    // If this is already the outermost context
    // let's not waste any time, because there's
    // nothing out there to draw from.
    if(_pParent == NULL)
        return;

    // Is there a pending notification?
    // If this assertion fires, it's probably because
    // of a missing CommonParserTextRun call just beforehand;
    // theoretically, the markup that introduces a
    // recursive structure should also have caused
    // the previous TextRun to have notified the sink.
    assert(_pParent->IsReset());
    // Contents aren't pushed.  Theoretically, there
    // shouldn't even be any.

    _oLocation.Push(_pParent->_oLocation);
    _oStyle.Push(_pParent->_oStyle);
    _oTransform.Push(_pParent->_oTransform);
}

void 
CommonParserTextRunElement::_Push(CommonParserEnvironment* pEnv)
{
    if(pEnv == NULL)
        return;

    // Contents aren't pushed.  Theoretically, there
    // shouldn't even be any.

    _oLocation.Push(pEnv);
    _oStyle.Push(pEnv);
    _oTransform.Push(pEnv);
}

// This method identifies the differences between the inner (this)
// and outer (parent) context.  It "emphasizes" those differences
// by asserting them in the outer context's "delta" list.  Really,
// it's just self-reporting what's already out there, but allows
// that outer "delta" list to be present and correct.
void 
CommonParserTextRunElement::_Pop()
{
    // If this is already the outermost context
    // let's not waste any time reporting out
    // deltas.
    if(_pParent == NULL)
        return;

    // Is there a pending notification?
    // If this assertion fires, it's probably because
    // of a missing CommonParserTextRun call just beforehand;
    // theoretically, the markup that introduces a
    // recursive structure should also have caused
    // the previous TextRun to have notified the sink.
    assert(IsReset());

    if(_pParent) {
        _oLocation.Pop(_pParent->_oLocation);
        _oStyle.Pop(_pParent->_oStyle);
        _oTransform.Pop(_pParent->_oTransform);
    }
}
PXR_NAMESPACE_CLOSE_SCOPE
