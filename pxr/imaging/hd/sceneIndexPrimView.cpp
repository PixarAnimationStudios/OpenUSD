//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSceneIndexPrimView::const_iterator &
HdSceneIndexPrimView::const_iterator::operator++()
{
    if (_skipDescendants) {
        _skipDescendants = false;
    } else {
        SdfPathVector children =
            _inputSceneIndex->GetChildPrimPaths(**this);
        if (!children.empty()) {
            if (_order == Lexicographic) {
                std::sort(children.begin(), children.end());
            }
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

HdSceneIndexPrimView::const_iterator::const_iterator(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &root,
        const Order order)
  : _inputSceneIndex(inputSceneIndex)
  , _stack{{{root},0}}
  , _skipDescendants(false)
  , _order(order)
{
}

HdSceneIndexPrimView::const_iterator::const_iterator(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const Order order)
  : _inputSceneIndex(inputSceneIndex)
  , _skipDescendants(false)
  , _order(order)
{
}

HdSceneIndexPrimView::HdSceneIndexPrimView(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const Order order)
  : HdSceneIndexPrimView(
      inputSceneIndex, SdfPath::AbsoluteRootPath(), order)
{
}

HdSceneIndexPrimView::HdSceneIndexPrimView(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &root,
        const Order order)
  : _begin(inputSceneIndex, root, order)
  , _end(inputSceneIndex, order)
{
}

const HdSceneIndexPrimView::const_iterator &
HdSceneIndexPrimView::begin() const
{
    return _begin;
}

const HdSceneIndexPrimView::const_iterator &
HdSceneIndexPrimView::end() const
{
    return _end;
}

PXR_NAMESPACE_CLOSE_SCOPE
