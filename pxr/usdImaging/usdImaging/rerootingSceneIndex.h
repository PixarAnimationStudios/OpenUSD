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
#ifndef PXR_USD_IMAGING_USD_IMAGING_REROOTING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_REROOTING_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingRerootingSceneIndex);

/// UsdImagingRerootingSceneIndex
///
/// Drops all prims not under srcPrefix and moves those under srcPrefix to
/// dstPrefix.
///
/// Data sources containing paths will be updated accordingly. That is, if it
/// contains a path with srcPrefix as prefix, the prefix will be replaced by
/// dstPrefix.
///
/// Note that this can be used as prefixing scene index by setting srcPrefix
/// to the root path. It can also use to isolate part of the namespace by
/// setting the srcPrefix and dstPrefix to be equal.
///
class UsdImagingRerootingSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static UsdImagingRerootingSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix)
    {
        return TfCreateRefPtr(
            new UsdImagingRerootingSceneIndex(
                inputScene, srcPrefix, dstPrefix));
    }

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    USDIMAGING_API
    UsdImagingRerootingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix);

    USDIMAGING_API
    ~UsdImagingRerootingSceneIndex() override;

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    SdfPath _SrcPathToDstPath(const SdfPath &primPath) const;
    SdfPath _DstPathToSrcPath(const SdfPath &primPath) const;

    const SdfPath _srcPrefix;
    const SdfPath _dstPrefix;
    // Prefixes of _dstPrefix
    const SdfPathVector _dstPrefixes;
    // Is _srcPrefix equal to _dstPrefix?
    const bool _srcEqualsDst;
    // Is _srcPrefix == / ?
    const bool _srcPrefixIsRoot;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
