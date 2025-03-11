//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/usdImaging/usdImaging/materialBindingsResolvingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/collectionMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"

#include "pxr/usd/usdShade/tokens.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionSchema.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Container that computes the resolved material binding from the flattened 
// direct material bindings.
//
// XXX The flattened direct binding is returned as the resolved binding.
//     This needs to be updated to factor collection bindings.
// 
class _HdMaterialBindingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_HdMaterialBindingsDataSource);

    _HdMaterialBindingsDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _primContainer(primContainer)
    , _si(si)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        return UsdImagingMaterialBindingsSchema::GetFromParent(
            _primContainer).GetPurposes();

        // Note: We don't check for collection membership here since it can be
        //       expensive and would involve pulling on bindings for purposes
        //       the renderer may not be interested in.
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        const TfToken &purpose = name;

        const UsdImagingMaterialBindingVectorSchema bindingVecSchema =
            UsdImagingMaterialBindingsSchema::GetFromParent(
                _primContainer).GetMaterialBindings(purpose);
        
        const SdfPath winningBindingPath =
            _ComputeResolvedMaterialBinding(bindingVecSchema);

        if (TfDebug::IsEnabled(USDIMAGING_MATERIAL_BINDING_RESOLUTION)) {
            if (!winningBindingPath.IsEmpty()) {
                TfDebug::Helper().Msg(
                    "*** Prim <%s>: Resolved material binding for purpose "
                    "\'%s\' is <%s>.\n", _primPath.GetText(),
                    purpose.IsEmpty()? "allPurpose": purpose.GetText(),
                    winningBindingPath.GetText());
            }
        }

        return _BuildHdMaterialBindingDataSource(winningBindingPath);
        
        // Note: If the resolved path is the empty path, we don't fallback to 
        //       checking/returning the binding for the empty (allPurpose)
        //       token, with the rationale being that a downstream scene index 
        //       plugin enumerates the strength of the material binding purposes
        //       using for e.g. HdsiMaterialBindingResolvingSceneIndex.
    }

private:

    struct _ResolveInfo
    {
        SdfPath materialPath;
        bool strongerThanDescendants;
        std::optional<SdfPath> collectionPath;
    };

    std::optional<_ResolveInfo>
    _ComputeResolveInfo(
        const UsdImagingCollectionMaterialBindingVectorSchema &colVecSchema)
        const
    {
        // Return the first collection binding that affects the prim.
        //
        for (size_t j = 0; j < colVecSchema.GetNumElements(); j++) {
            const UsdImagingCollectionMaterialBindingSchema
                colBindingSchema = colVecSchema.GetElement(j);
            
            auto colPathDs    = colBindingSchema.GetCollectionPath();
            auto matPathDs    = colBindingSchema.GetMaterialPath();
            auto strengthDs   = colBindingSchema.GetBindingStrength();

            if (!(colPathDs && matPathDs && strengthDs)) {
                continue;
            }

            // Path returned will be of the form /Foo.collection:colName
            const SdfPath collectionAttributePath =
                colPathDs->GetTypedValue(0.0);

            // Query scene index to get collection path expression.
            auto exprOpt = _GetCollectionPathExpression(
                collectionAttributePath);
            if (!exprOpt) {
                continue;
            }

            auto eval = HdCollectionExpressionEvaluator(_si, *exprOpt);
            // XXX This does not handle instance proxy paths yet.
            if (!eval.Match(_primPath)) {
                TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                "- Prim <%s> is NOT affected by collection material binding "
                "<%s> (expr = \"%s\").\n", _primPath.GetText(),
                collectionAttributePath.GetText(), exprOpt->GetText().c_str());

                continue;
            }

            TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                "+ Prim <%s> IS affected by collection material binding <%s> "
                "(expr = \"%s\").\n", _primPath.GetText(),
                collectionAttributePath.GetText(), exprOpt->GetText().c_str());

            return
                _ResolveInfo {
                    matPathDs->GetTypedValue(0.0),
                    strengthDs->GetTypedValue(0.0) ==
                        UsdShadeTokens->strongerThanDescendants,
                    collectionAttributePath};
        }

        return std::nullopt;
    }

    std::optional<_ResolveInfo>
    _ComputeResolveInfo(
        const UsdImagingDirectMaterialBindingSchema &dirBindingSchema)
        const
    {
        auto dirBindingMatPathDs = dirBindingSchema.GetMaterialPath();
        auto dirBindingStrengthDs = dirBindingSchema.GetBindingStrength();

        if (dirBindingMatPathDs && dirBindingStrengthDs) {
            return
                _ResolveInfo{
                    dirBindingMatPathDs->GetTypedValue(0.0),
                    dirBindingStrengthDs->GetTypedValue(0.0) ==
                        UsdShadeTokens->strongerThanDescendants,
                    /* collectionPath */ std::nullopt};
        }

        return std::nullopt;   
    }

    SdfPath
    _ComputeResolvedMaterialBinding(
        const UsdImagingMaterialBindingVectorSchema &bindingVecSchema) const
    {
        TRACE_FUNCTION();

        // The input is a vector of {direct, collection} binding pairs.
        // The elements are ordered as in a DFS traversal with ancestors
        // appearing before descendants. So, if we find a binding with a
        // strongerThanDescendants strength, we can skip the rest of the
        // bindings.
        //
        SdfPath winningBindingPath;

        for (size_t i = 0; i < bindingVecSchema.GetNumElements(); i++) {
            const UsdImagingMaterialBindingSchema bindingSchema =
                bindingVecSchema.GetElement(i);
            
            std::optional<_ResolveInfo> colBindInfo = _ComputeResolveInfo(
                bindingSchema.GetCollectionMaterialBindings());

            if (colBindInfo && colBindInfo->strongerThanDescendants) {
                winningBindingPath = colBindInfo->materialPath;

                TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                    "Prim <%s>: Winning material set to <%s>. "
                    "Binding strength for collection binding "
                    "<%s> is strongerThanDescendants. "
                    "Skipping the rest of the bindings.\n",
                    _primPath.GetText(),
                    winningBindingPath.GetText(),
                    colBindInfo->collectionPath->GetText());

                break;
            }

            std::optional<_ResolveInfo> dirBindInfo = _ComputeResolveInfo(
                    bindingSchema.GetDirectMaterialBinding());

            if (dirBindInfo && dirBindInfo->strongerThanDescendants) {
                winningBindingPath = dirBindInfo->materialPath;

                TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                    "Prim <%s>: Winning material set to <%s>. "
                    "Binding strength for direct binding "
                    "is strongerThanDescendants. "
                    "Skipping the rest of the bindings.\n",
                    _primPath.GetText(), winningBindingPath.GetText());
            
                break;
            }

            if (colBindInfo ) {
                // Neither of the bindings is stronger than descendants.
                // The collection binding is considered stronger than the direct
                // binding at any namespace level.
                //
                winningBindingPath = colBindInfo->materialPath;

                TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                    "Prim <%s>: Current winning material set to <%s> for "
                    "collection binding <%s>.\n",
                    _primPath.GetText(),
                    winningBindingPath.GetText(),
                    colBindInfo->collectionPath->GetText());
                
                continue;
            }

            if (dirBindInfo) {
                // No collection binding was found, so the direct binding
                // wins. We still need to iterate over the rest of the
                // bindings.
                //
                winningBindingPath = dirBindInfo->materialPath;

                TF_DEBUG(USDIMAGING_MATERIAL_BINDING_RESOLUTION).Msg(
                    "Prim <%s>: Current winning material set to <%s> "
                    "because the direct binding is more local.\n",
                    _primPath.GetText(), winningBindingPath.GetText());
            }
        }
  
        return winningBindingPath;
    }

    std::optional<SdfPathExpression>
    _GetCollectionPathExpression(const SdfPath &collectionAttributePath) const
    {
        const SdfPath primPath = collectionAttributePath.GetPrimPath();
        const auto [collectionName, namespaceFound] =
            SdfPath::StripPrefixNamespace(
                collectionAttributePath.GetName(),
                HdCollectionSchemaTokens->collection.GetString());

        if (!namespaceFound) {
            return std::nullopt;
        }

        HdContainerDataSourceHandle primDs = _si->GetPrim(primPath).dataSource;
        HdCollectionSchema colSchema =
            HdCollectionsSchema::GetFromParent(primDs)
            .GetCollection(TfToken(collectionName));
        
        const auto  exprDs = colSchema.GetMembershipExpression();
        if (!exprDs) {
            return std::nullopt;
        }

        return exprDs->GetTypedValue(0.0);
    }

    static HdDataSourceBaseHandle
    _BuildHdMaterialBindingDataSource(const SdfPath &materialPath)
    {
        return
            materialPath.IsEmpty()
            ? nullptr
            : HdMaterialBindingSchema::Builder()
                .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(
                    materialPath))
                .Build();
    }

private:
    HdContainerDataSourceHandle _primContainer;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
};

// Prim container override that provides the resolved hydra material bindings
// if direct or collection USD material bindings are present.
// 
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _primContainer(primContainer)
    , _si(si)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _primContainer->GetNames();
        names.push_back(HdMaterialBindingsSchema::GetSchemaToken());
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _primContainer->Get(name);

        // Material bindings on the prim.
        if (name == HdMaterialBindingsSchema::GetSchemaToken()) {

            // Check if we have USD material bindings on the prim to
            // avoid returning an empty non-null container.
            if (UsdImagingMaterialBindingsSchema::GetFromParent(
                _primContainer)) {
                // We don't expect to have hydra material bindings on the
                // prim container. Use an overlay just in case such that the
                // existing opinion wins.
                return
                    HdOverlayContainerDataSource::New(
                        HdContainerDataSource::Cast(result),
                        _HdMaterialBindingsDataSource::New(
                            _primContainer, _si, _primPath));
            }
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _primContainer;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
};

}

// -----------------------------------------------------------------------------
// UsdImagingMaterialBindingsResolvingSceneIndex
// -----------------------------------------------------------------------------

UsdImagingMaterialBindingsResolvingSceneIndexRefPtr
UsdImagingMaterialBindingsResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new UsdImagingMaterialBindingsResolvingSceneIndex(
            inputSceneIndex, inputArgs));
}

UsdImagingMaterialBindingsResolvingSceneIndex::
UsdImagingMaterialBindingsResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

UsdImagingMaterialBindingsResolvingSceneIndex::
~UsdImagingMaterialBindingsResolvingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingMaterialBindingsResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Wrap the prim container to provide the resolved hydra bindings via
    // the "materialBindings" locator.
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource =
            _PrimDataSource::New(
                prim.dataSource, _GetInputSceneIndex(), primPath);
    }

    return prim;
}

SdfPathVector
UsdImagingMaterialBindingsResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // This scene index does not mutate the topology.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // For now, just forward the notices. We could suppress notices
    // for USD material bindings schemata locators since scene indices
    // downstream shouldn't be interested in these notices.
    //
    _SendPrimsAdded(entries);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Comments above in _PrimsAdded are relevant here.
    _SendPrimsRemoved(entries);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Check if the notice entries can be forwarded as-is.
    bool hasDirtyUsdMaterialBindings = false;
    for (auto const &entry : entries) {
        if (entry.dirtyLocators.Intersects(
                UsdImagingMaterialBindingsSchema::GetDefaultLocator())) {
            hasDirtyUsdMaterialBindings = true;
            break;
        }
    }

    if (!hasDirtyUsdMaterialBindings) {
        _SendPrimsDirtied(entries);
        return;
    }

    // Transform dirty notices for USD material bindings into ones for
    // Hydra material bindings. This effectively suppresses the former notices,
    // which is fine because downstream consumers should work off the
    // Hydra material binding notices.
    //
    HdSceneIndexObserver::DirtiedPrimEntries newEntries;
    for (auto const &entry : entries) {
         if (entry.dirtyLocators.Intersects(
            UsdImagingMaterialBindingsSchema::GetDefaultLocator())) {

            HdDataSourceLocatorSet newLocators(entry.dirtyLocators);
            newLocators = newLocators.ReplacePrefix(
                UsdImagingMaterialBindingsSchema::GetDefaultLocator(),
                HdMaterialBindingsSchema::GetDefaultLocator());

            newEntries.push_back({entry.primPath, newLocators});
        } else {
            newEntries.push_back(entry);
        }
    }

    _SendPrimsDirtied(newEntries);
}


PXR_NAMESPACE_CLOSE_SCOPE
