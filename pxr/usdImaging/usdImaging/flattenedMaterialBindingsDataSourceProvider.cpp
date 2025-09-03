//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/flattenedMaterialBindingsDataSourceProvider.h"

#include "pxr/usdImaging/usdImaging/materialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"

#include "pxr/base/tf/denseHashSet.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Aggregate material bindings from the parent with the prim's local bindings.
//
class _MaterialBindingsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialBindingsDataSource);

    TfTokenVector GetNames() override {
        TfDenseHashSet<TfToken, TfToken::HashFunctor> allPurposes;
        for (const TfTokenVector &purposes :
                {_primBindings->GetNames(), _parentBindings->GetNames()} ) {
            allPurposes.insert(purposes.begin(), purposes.end());
        }

        return {allPurposes.begin(), allPurposes.end()};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TF_AXIOM(_primBindings && _parentBindings);
        const TfToken &purpose = name;
                
        const auto parentSchema =
            UsdImagingMaterialBindingsSchema(_parentBindings);
        UsdImagingMaterialBindingVectorSchema parentBindingsSchema =
                parentSchema.GetMaterialBindings(purpose);

        const auto primSchema =
            UsdImagingMaterialBindingsSchema(_primBindings);
        UsdImagingMaterialBindingVectorSchema primBindingsSchema =
                primSchema.GetMaterialBindings(purpose);

        if (parentBindingsSchema.GetNumElements() == 0) {
            return primBindingsSchema.GetVector();
        }

        if (primBindingsSchema.GetNumElements() == 0) {
            return parentBindingsSchema.GetVector();
        }

        // Insert the prim's opinion after the parent's. The binding resolving
        // scene index walks through the bindings in this order to short
        // circuit membership evaluation when possible.
        return _Concat(
            parentBindingsSchema.GetVector(), primBindingsSchema.GetVector());
    }

    // Returns a vector data source that concatenates valid elements of the
    // given vector data sources.
    //
    static
    HdVectorDataSourceHandle
    _Concat(
        const HdVectorDataSourceHandle &a, const HdVectorDataSourceHandle &b)
    {
        const size_t numA = a->GetNumElements();
        const size_t numB = b->GetNumElements();
        std::vector<HdDataSourceBaseHandle> result;
        result.reserve(numA + numB);

        for (size_t i = 0; i < numA; ++i) {
            if (const auto ds = a->GetElement(i)) {
                result.push_back(ds);
            }
        }
        for (size_t i = 0; i < numB; ++i) {
            if (const auto ds = b->GetElement(i)) {
                result.push_back(ds);
            }

        }

        return HdRetainedSmallVectorDataSource::New(
            result.size(), result.data());
    }

    // Return data source with the correct composition behavior.
    //
    // This avoids allocating the aggregated data source if only one
    // of the given handles is non-null (which we expect to be the general
    // case).
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
UsdImagingFlattenedMaterialBindingsDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    return 
        _MaterialBindingsDataSource::UseOrCreateNew(
            ctx.GetInputDataSource(),
            ctx.GetFlattenedDataSourceFromParentPrim());
}

void
UsdImagingFlattenedMaterialBindingsDataSourceProvider::
ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    // Any locator of the form BindingPurpose:Foo:... will be turned into
    // BindingPurpose. The data source for the aggregated bindings needs
    // to be recomputed.
    bool modifyInputLocators = false;

    for (const HdDataSourceLocator &locator : *locators) {
        if (locator.GetElementCount() != 1) {
            modifyInputLocators = true;
            break;
        }
    }
    if (!modifyInputLocators) {
        return; // Use input locators set.
    }
     
    HdDataSourceLocatorSet result;
    for (const HdDataSourceLocator &locator : *locators) {
        result.insert(HdDataSourceLocator(locator.GetFirstElement()));
    }
    *locators = std::move(result);
}

PXR_NAMESPACE_CLOSE_SCOPE
