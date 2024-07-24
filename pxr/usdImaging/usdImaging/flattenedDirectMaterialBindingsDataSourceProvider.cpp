//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/flattenedDirectMaterialBindingsDataSourceProvider.h"

#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingsSchema.h"

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
        TfDenseHashSet<TfToken, TfToken::HashFunctor> allPurposes;
        {
            for (const TfTokenVector &purposes :
                    {_primBindings->GetNames(), _parentBindings->GetNames()} ) {
                allPurposes.insert(purposes.begin(), purposes.end());
            }
        }

        return { allPurposes.begin(), allPurposes.end() };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        const TfToken &purpose = name;
        
        UsdImagingDirectMaterialBindingSchema parentSchema(
            HdContainerDataSource::Cast(_parentBindings->Get(purpose)));

        if (HdTokenDataSourceHandle const strengthDs =
                parentSchema.GetBindingStrength()) {
            const TfToken strength = strengthDs->GetTypedValue(0.0f);
            if (strength == _tokens->strongerThanDescendants) {
                return parentSchema.GetContainer();
            }
        }
        if (HdDataSourceBaseHandle const bindingDs =
                _primBindings->Get(purpose)) {
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
UsdImagingFlattenedDirectMaterialBindingsDataSourceProvider::
GetFlattenedDataSource(
    const Context &ctx) const
{
    return 
        _MaterialBindingsDataSource::UseOrCreateNew(
            ctx.GetInputDataSource(),
            ctx.GetFlattenedDataSourceFromParentPrim());
}

void
UsdImagingFlattenedDirectMaterialBindingsDataSourceProvider::
ComputeDirtyLocatorsForDescendants(
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
