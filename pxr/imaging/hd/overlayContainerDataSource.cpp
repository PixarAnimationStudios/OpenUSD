//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/overlayContainerDataSource.h"

#include "pxr/base/tf/denseHashSet.h"

PXR_NAMESPACE_OPEN_SCOPE

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const std::initializer_list<HdContainerDataSourceHandle> &sources)
 : _sources(sources.begin(), sources.end())
{
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const size_t count,
    HdContainerDataSourceHandle * const sources)
{
    _sources.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        _sources.push_back(sources[i]);
    }
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    _ContainerVector &&sources)
 : _sources(std::move(sources))
{
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const HdContainerDataSourceHandle &src1,
    const HdContainerDataSourceHandle &src2)
{
    _sources = { src1, src2 };
}

HdOverlayContainerDataSource::HdOverlayContainerDataSource(
    const HdContainerDataSourceHandle &src1,
    const HdContainerDataSourceHandle &src2,
    const HdContainerDataSourceHandle &src3)
{
    _sources = { src1, src2, src3 };
}

HdContainerDataSourceHandle
HdOverlayContainerDataSource::OverlayedContainerDataSources(
        const HdContainerDataSourceHandle &src1,
        const HdContainerDataSourceHandle &src2)
{
    return HdCreateOverlayContainerDataSource(src1, src2);
}

TfTokenVector
HdOverlayContainerDataSource::GetNames()
{
    TfDenseHashSet<TfToken, TfToken::HashFunctor> usedNames;
    for (HdContainerDataSourceHandle const &c : _sources) {
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
    TfSmallVector<HdContainerDataSourceHandle, 8> childSources;

    for (HdContainerDataSourceHandle const &c : _sources) {
        if (c) {
            if (HdDataSourceBaseHandle child = c->Get(name)) {
                if (auto childContainer = HdContainerDataSource::Cast(child)) {
                    childSources.push_back(childContainer);
                } else {

                    // if there are already sources to our left, we should
                    // return those rather than replace it with a non-container
                    // value
                    if (!childSources.empty()) {
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

    switch (childSources.size())
    {
    case 0:
        return nullptr;
    case 1:
        return childSources[0];
    default:
        return New(std::move(childSources));
    }
}

namespace
{

// Avoid making HdOverlayContainerDataSource::_ContainerVector public
// or adding friends by simply repeating it here:
using _ContainerVector = TfSmallVector<HdContainerDataSourceHandle, 8>;
    
}

HdContainerDataSourceHandle
HdCreateOverlayContainerDataSource(
    const size_t count,
    HdContainerDataSourceHandle * const sources)
{
    _ContainerVector nonNullSources;
    nonNullSources.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        if (sources[i]) {
            nonNullSources.push_back(sources[i]);
        }
    }
    switch (nonNullSources.size()) {
    case 0:
        return nullptr;
    case 1:
        return nonNullSources[0];
    default:
        return HdOverlayContainerDataSource::New(std::move(nonNullSources));
    }
}

HdContainerDataSourceHandle
HdCreateOverlayContainerDataSource(
    const std::initializer_list<HdContainerDataSourceHandle> &sources)
{
    _ContainerVector nonNullSources;
    nonNullSources.reserve(sources.size());
    for (HdContainerDataSourceHandle const &source : sources) {
        if (source) {
            nonNullSources.push_back(source);
        }
    }
    switch (nonNullSources.size()) {
    case 0:
        return nullptr;
    case 1:
        return nonNullSources[0];
    default:
        return HdOverlayContainerDataSource::New(std::move(nonNullSources));
    }
}

HdContainerDataSourceHandle
HdCreateOverlayContainerDataSource(
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

PXR_NAMESPACE_CLOSE_SCOPE

