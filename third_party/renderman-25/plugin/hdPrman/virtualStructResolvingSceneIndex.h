//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VIRTUAL_STRUCT_RESOLVING_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VIRTUAL_STRUCT_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdPrman_VirtualStructResolvingSceneIndex);

class HdPrman_VirtualStructResolvingSceneIndex :
    public HdMaterialFilteringSceneIndexBase
{
public:

    static HdPrman_VirtualStructResolvingSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputScene, bool applyConditionals=true) 
    {
        return TfCreateRefPtr(
            new HdPrman_VirtualStructResolvingSceneIndex(
                inputScene, applyConditionals));
    }

protected:
    HdPrman_VirtualStructResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex, bool applyConditionals);

    FilteringFnc _GetFilteringFunction() const override;

private:
    bool _applyConditionals;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VIRTUAL_STRUCT_RESOLVING_SCENE_INDEX_H
