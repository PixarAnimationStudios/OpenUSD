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
#include "pxr/imaging/hd/flattenedMaterialBindingsDataSourceProvider.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"

#include "pxr/base/tf/denseHashSet.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (strongerThanDescendants)
);

namespace {

// Parent and local bindings might have unique fields so we must
// overlay them. If we are concerned about overlay depth, we could
// compare GetNames() results to decide whether the child bindings
// completely mask the parent.
//
// Like an HdOverlayContainerDataSource, but looking at bindingStrength
// to determine which data source is stronger.
//
class _MaterialBindingsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialBindingsDataSource);

    TfTokenVector GetNames() override {
        TfDenseHashSet<TfToken, TfToken::HashFunctor> allNames;
        {
            for (const TfTokenVector &names :
                    {_primBindings->GetNames(), _parentBindings->GetNames()} ) {
                allNames.insert(names.begin(), names.end());
            }
        }

        return { allNames.begin(), allNames.end() };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        HdMaterialBindingSchema parentSchema(
            HdContainerDataSource::Cast(
                _parentBindings->Get(name)));
        if (HdTokenDataSourceHandle const strengthDs =
                parentSchema.GetBindingStrength()) {
            const TfToken strength = strengthDs->GetTypedValue(0.0f);
            if (strength == _tokens->strongerThanDescendants) {
                return parentSchema.GetContainer();
            }
        }
        if (HdDataSourceBaseHandle const bindingDs = _primBindings->Get(name)) {
            return bindingDs;
        }
        return parentSchema.GetContainer();
    }

    // Return data source with the correct composition behavior.
    //
    // This avoids allocating the _MaterialBindingsDataSource if only one
    // of the given handles is non-null.
    static
    HdContainerDataSourceHandle
    UseOrCreateNew(
        HdContainerDataSourceHandle const &primBindings,
        HdContainerDataSourceHandle const &parentBindings)
    {
        if (!primBindings) {
            return parentBindings;
        }
        if (!parentBindings) {
            return primBindings;
        }
        return New(primBindings, parentBindings);
    }

private:
    _MaterialBindingsDataSource(
        HdContainerDataSourceHandle const &primBindings,
        HdContainerDataSourceHandle const &parentBindings)
      : _primBindings(primBindings)
      , _parentBindings(parentBindings)
    {
    }

    HdContainerDataSourceHandle const _primBindings;
    HdContainerDataSourceHandle const _parentBindings;
};

}

HdContainerDataSourceHandle
HdFlattenedMaterialBindingsDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    return 
        _MaterialBindingsDataSource::UseOrCreateNew(
            ctx.GetInputDataSource(),
            ctx.GetFlattenedDataSourceFromParentPrim());
}

void
HdFlattenedMaterialBindingsDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    // Any locator of the form BindingPurpose:Foo will be turned into
    // BindingPurpose.
    //
    // The reason. Foo could be bindingStrength and thus affect
    // BindingPurpose:Path.

    bool needsTransform = false;

    for (const HdDataSourceLocator &locator : *locators) {
        if (locator.GetElementCount() != 1) {
            needsTransform = true;
            break;
        }
    }
    if (!needsTransform) {
        return;
    }
     
    HdDataSourceLocatorSet result;
    for (const HdDataSourceLocator &locator : *locators) {
        result.insert(HdDataSourceLocator(locator.GetFirstElement()));
    }
    *locators = std::move(result);
}

PXR_NAMESPACE_CLOSE_SCOPE
