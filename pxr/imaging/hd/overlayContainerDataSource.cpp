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
#include "pxr/imaging/hd/overlayContainerDataSource.h"

#include "pxr/base/tf/denseHashSet.h"

PXR_NAMESPACE_OPEN_SCOPE

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    std::initializer_list<HdContainerDataSourceHandle> sources)
    : _containers(sources.begin(), sources.end())
{
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    size_t count,
    HdContainerDataSourceHandle *containers)
{
    _containers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        _containers.push_back(containers[i]);
    }
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const HdContainerDataSourceHandle &src1,
    const HdContainerDataSourceHandle &src2)
{
    _containers = { src1, src2 };
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const HdContainerDataSourceHandle &src1,
    const HdContainerDataSourceHandle &src2,
    const HdContainerDataSourceHandle &src3)
{
    _containers = { src1, src2, src3 };
}

HdContainerDataSourceHandle
HdOverlayContainerDataSource::OverlayedContainerDataSources(
        const HdContainerDataSourceHandle &src1,
        const HdContainerDataSourceHandle &src2)
{
    if (!src1) {
        return src2;
    }
    if (!src2) {
        return src1;
    }
    return HdOverlayContainerDataSource::New(src1, src2);
}


TfTokenVector
HdOverlayContainerDataSource::GetNames()
{
    TfDenseHashSet<TfToken, TfToken::HashFunctor> usedNames;
    for (HdContainerDataSourceHandle &c : _containers) {
        if (c) {
            for (const TfToken &name : c->GetNames()) {
                usedNames.insert(name);
            }
        }
    }

    return TfTokenVector(usedNames.begin(), usedNames.end());
}

HdDataSourceBaseHandle
HdOverlayContainerDataSource::Get(
    const TfToken &name)
{
    TfSmallVector<HdContainerDataSourceHandle, 8> childContainers;

    for (HdContainerDataSourceHandle const &c : _containers) {
        if (c) {
            if (HdDataSourceBaseHandle child = c->Get(name)) {
                if (auto childContainer = HdContainerDataSource::Cast(child)) {
                    childContainers.push_back(childContainer);
                } else {

                    // if there are already containers to our left, we should
                    // return those rather than replace it with a non-container
                    // value
                    if (!childContainers.empty()) {
                        break;
                    }

                    // HdBlockDataSource's role is to mask values
                    if (HdBlockDataSource::Cast(child)) {
                        return nullptr;
                    }
                    return child;
                }
            }
        }
    }

    switch (childContainers.size())
    {
    case 0:
        return nullptr;
    case 1:
        return childContainers[0];
    default:
        return New(childContainers.size(), childContainers.data());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

