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
#ifndef PXR_IMAGING_HD_SCENE_INDEX_PRIM_VIEW_H
#define PXR_IMAGING_HD_SCENE_INDEX_PRIM_VIEW_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

/// \class HdSceneIndexPrimView
///
/// A range to iterate over all descendants of a given prim (including
/// the prim itself) in a scene index in depth-first order.
/// The descendants of the current prim can be skipped by calling
/// SkipDescendants.
///
/// Example:
/// \code
///
/// for (const SdfPath &primPath :
///           HdSceneIndexPrimView(mySceneIndex, myRootPath)) {
///     ...
/// }
///
/// HdSceneIndexPrimView view(mySceneIndex, myRootPath);
/// for (auto it = view.begin(); it != view.end(); ++it) {
///      const SdfPath &primPath = *it;
///      ...
///      if (_ShouldSkipDescendants(primPath)) {
///          it.SkipDescendants();
///      }
/// }
///
/// \endcode
///
class HdSceneIndexPrimView
{
public:
    class const_iterator
    {
    public:
        inline const SdfPath &operator*() const;

        HD_API
        const_iterator& operator++();

        inline void SkipDescendants();
        inline bool operator==(const const_iterator &other) const;
        inline bool operator!=(const const_iterator &other) const;

    private:
        friend class HdSceneIndexPrimView;
        struct _StackFrame;

        const_iterator(HdSceneIndexBaseRefPtr const &inputSceneIndex,
                       const SdfPath &root);
        const_iterator(HdSceneIndexBaseRefPtr const &inputSceneIndex);

        HdSceneIndexBaseRefPtr const _inputSceneIndex;
        std::vector<_StackFrame> _stack;
        bool _skipDescendants;
    };

    HD_API
    HdSceneIndexPrimView(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    HD_API
    HdSceneIndexPrimView(HdSceneIndexBaseRefPtr const &inputSceneIndex,
                         const SdfPath &root);

    HD_API
    const const_iterator &begin() const;

    HD_API
    const const_iterator &end() const;

private:
    const const_iterator _begin;
    const const_iterator _end;
};

struct
HdSceneIndexPrimView::const_iterator::_StackFrame
{
    std::vector<SdfPath> paths;
    size_t index;
    
    bool operator==(const _StackFrame &other) const {
        return paths == other.paths && index == other.index;
    }
};

const SdfPath &
HdSceneIndexPrimView::const_iterator::operator*() const
{
    const _StackFrame &frame = _stack.back();
    return frame.paths[frame.index];
}

void
HdSceneIndexPrimView::const_iterator::SkipDescendants()
{
    _skipDescendants = true;
}

bool
HdSceneIndexPrimView::const_iterator::operator==(
    const const_iterator &other) const
{
    return _stack == other._stack;
}

bool
HdSceneIndexPrimView::const_iterator::operator!=(
    const const_iterator &other) const
{
    return !(*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
