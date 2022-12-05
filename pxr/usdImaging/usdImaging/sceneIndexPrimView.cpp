//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/sceneIndexPrimView.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImaging_SceneIndexPrimView::const_iterator &
UsdImaging_SceneIndexPrimView::const_iterator::operator++()
{
    if (_skipDescendants) {
        _skipDescendants = false;
    } else {
        SdfPathVector children =
            _inputSceneIndex->GetChildPrimPaths(**this);
        if (!children.empty()) {
            _stack.push_back({std::move(children), 0});
            return *this;
        }
    }
    
    while (!_stack.empty()) {
        _StackFrame &frame = _stack.back();
        ++frame.index;
        if (frame.index < frame.paths.size()) {
            break;
        }
        _stack.pop_back();
    }
    
    return *this;
}

UsdImaging_SceneIndexPrimView::const_iterator::const_iterator(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &root)
  : _inputSceneIndex(inputSceneIndex)
  , _stack{{{root},0}}
  , _skipDescendants(false)
{
}

UsdImaging_SceneIndexPrimView::const_iterator::const_iterator(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : _inputSceneIndex(inputSceneIndex)
  , _skipDescendants(false)
{
}

UsdImaging_SceneIndexPrimView::UsdImaging_SceneIndexPrimView(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &root)
  : _begin(inputSceneIndex, root)
  , _end(inputSceneIndex)
{
}

const UsdImaging_SceneIndexPrimView::const_iterator &
UsdImaging_SceneIndexPrimView::begin() const
{
    return _begin;
}

const UsdImaging_SceneIndexPrimView::const_iterator &
UsdImaging_SceneIndexPrimView::end() const
{
    return _end;
}

PXR_NAMESPACE_CLOSE_SCOPE
