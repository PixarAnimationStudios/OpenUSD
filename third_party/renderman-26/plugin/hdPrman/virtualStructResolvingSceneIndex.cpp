//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/virtualStructResolvingSceneIndex.h"
#include "hdPrman/matfiltResolveVstructs.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

static void
_ResolveVirtualStructsWithConditionals(
    HdMaterialNetworkInterface *networkInterface)
{
    MatfiltResolveVstructs(networkInterface, true);
}

static void
_ResolveVirtualStructsWithoutConditionals(
    HdMaterialNetworkInterface *networkInterface)
{
    MatfiltResolveVstructs(networkInterface, false);
}

HdPrman_VirtualStructResolvingSceneIndex::HdPrman_VirtualStructResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex, bool applyConditionals)
: HdMaterialFilteringSceneIndexBase(inputSceneIndex)
, _applyConditionals(applyConditionals)
{
}

HdMaterialFilteringSceneIndexBase::FilteringFnc
HdPrman_VirtualStructResolvingSceneIndex::_GetFilteringFunction() const
{
    // could capture _applyCondition but instead use wrapper function
    if (_applyConditionals) {
        return _ResolveVirtualStructsWithConditionals;
    } else {
        return _ResolveVirtualStructsWithoutConditionals;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
