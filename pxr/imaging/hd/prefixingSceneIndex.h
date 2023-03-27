//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_PREFIXING_SCENE_INDEX_H
#define PXR_IMAGING_HD_PREFIXING_SCENE_INDEX_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrefixingSceneIndex;
TF_DECLARE_REF_PTRS(HdPrefixingSceneIndex);

///
/// \class HdPrefixingSceneIndex
///
/// A prefixing scene index is one in which the input scene contains 
/// data sources whose paths are all prefixed with a given prefix.
///
class HdPrefixingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:

    /// Creates a new prefixing scene index.
    ///
    static HdPrefixingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene, const SdfPath &prefix) 
    {
        return TfCreateRefPtr(new HdPrefixingSceneIndex(inputScene, prefix));
    }

    // satisfying HdSceneIndexBase
    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HD_API
    HdPrefixingSceneIndex(const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &prefix);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:

    SdfPath _AddPathPrefix(const SdfPath &primPath) const;

    SdfPath _RemovePathPrefix(const SdfPath &primPath) const;

    const SdfPath _prefix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

