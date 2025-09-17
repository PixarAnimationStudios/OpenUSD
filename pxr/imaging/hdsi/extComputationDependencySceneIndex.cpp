//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/extComputationDependencySceneIndex.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (value)
    ((all, "__all__"))

    ((primvarExtComputationDependency, "primvarExtComputationDependency_"))

    (extComputationPrimvarsDependenciesDependency)

    (extComputationInputValuesDependency)
    ((extComputationInputDependency, "extComputationInputDependency_"))
    ((extComputationOutputDependency, "extComputationOutputDependency_"))

    (extComputationInputComputationsDependenciesDependency)
    (extComputationOutputsDependenciesDependency)
);

// Dependencies change when input computations of computation change.
HdDataSourceBaseHandle const &
_ExtComputationInputComputationsDependenciesDependency()
{
    static HdDataSourceBaseHandle const result =
        HdDependencySchema::Builder()
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdExtComputationSchema::GetInputComputationsLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator()))
            .Build();
    return result;
}

// Dependencies change when outputs of computation change - since we
// generate a dependency of each output on each input computation.
HdDataSourceBaseHandle const &
_ExtComputationOutputsDependenciesDependency()
{
    static HdDataSourceBaseHandle const result =
        HdDependencySchema::Builder()
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdExtComputationSchema::GetOutputsLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator()))
            .Build();
    return result;
}

// We have a dependency of each output on each input. To avoid adding
// this many depencencies, we funnel through this dummy locator.
HdLocatorDataSourceHandle const &
_AllOutputValuesLocatorDs()
{
    static HdLocatorDataSourceHandle const result =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdExtComputationSchema::GetOutputsLocator()
                .Append(_tokens->all)
                .Append(_tokens->value));
    return result;
}

// Add a dependency of all outputs (through dummy locator) on the
// inputValues of the computation.
HdDataSourceBaseHandle
_ExtComputationInputValuesDependency()
{
    static HdDataSourceBaseHandle const result =
        HdDependencySchema::Builder()
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdExtComputationSchema::GetInputValuesLocator()))
            .SetAffectedDataSourceLocator(
                _AllOutputValuesLocatorDs())
            .Build();
    return result;
}

// Build dependencies schema for an ext computation prim.
HdContainerDataSourceHandle
_BuildExtComputationDependencies(HdContainerDataSourceHandle const &primSource)
{
    TRACE_FUNCTION();

    TfTokenVector names;
    std::vector<HdDataSourceBaseHandle> sources;

    // All outputs depend on inputValues (achieved through dummy locator).
    names.push_back(_tokens->extComputationInputValuesDependency);
    sources.push_back(_ExtComputationInputValuesDependency());

    const auto compSchema =
        HdExtComputationSchema::GetFromParent(primSource);

    const HdExtComputationInputComputationContainerSchema
        inputComputationsSchema = compSchema.GetInputComputations();

    // Make all outputs depend on each input computation.
    for (const TfToken &inputName : inputComputationsSchema.GetNames()) {
        const HdExtComputationInputComputationSchema inputComputationSchema =
            inputComputationsSchema.Get(inputName);
        HdPathDataSourceHandle const sourceComputation =
            inputComputationSchema.GetSourceComputation();
        if (!sourceComputation) {
            continue;
        }
        HdTokenDataSourceHandle const sourceComputationOutputName =
            inputComputationSchema.GetSourceComputationOutputName();
        if (!sourceComputationOutputName) {
            continue;
        }

        names.push_back(
            TfToken(
                _tokens->extComputationInputDependency.GetString() +
                inputName.GetString()));
        sources.push_back(
            HdDependencySchema::Builder()
                // The ext computation prim corresponding to the input
                // computation.
                .SetDependedOnPrimPath(
                    sourceComputation)
                // The value of the output on that ext computation prim.
                //
                // Note that the locator does not correspond to an actual data
                // source in the scene index - but we can still use it to signal
                // to clients that the value of the computation (wherever it
                // will be executed) has changed.
                .SetDependedOnDataSourceLocator(
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdExtComputationSchema::GetOutputsLocator()
                            .Append(
                                sourceComputationOutputName->GetTypedValue(0.0f))
                            .Append(_tokens->value)))
                // Use dummy locator to affect the values of all outputs.
                .SetAffectedDataSourceLocator(
                    _AllOutputValuesLocatorDs())
                .Build());
    }

    const HdExtComputationOutputContainerSchema outputsSchema =
        compSchema.GetOutputs();

    // Make the value of each computation output depend on dummy locator.
    for (const TfToken &outputName : outputsSchema.GetNames()) {
        names.push_back(
            TfToken(
                _tokens->extComputationOutputDependency.GetString() +
                outputName.GetString()));
        sources.push_back(
            HdDependencySchema::Builder()
                .SetDependedOnDataSourceLocator(
                    _AllOutputValuesLocatorDs())
                .SetAffectedDataSourceLocator(
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdExtComputationSchema::GetOutputsLocator()
                            .Append(outputName)
                            .Append(_tokens->value)))
                .Build());
    }

    // Dependencies for the dependencies.

    names.push_back(_tokens->extComputationInputComputationsDependenciesDependency);
    sources.push_back(_ExtComputationInputComputationsDependenciesDependency());

    names.push_back(_tokens->extComputationOutputsDependenciesDependency);
    sources.push_back(_ExtComputationOutputsDependenciesDependency());

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), sources.data());
}

// Depenencies change when ext computation primvars change.
static HdDataSourceBaseHandle const &
_ExtComputationPrimvarsDependenciesDependency()
{
    static HdDataSourceBaseHandle const result =
        HdDependencySchema::Builder()
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdExtComputationPrimvarsSchema::GetDefaultLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator()))
            .Build();
    return result;
}

HdContainerDataSourceHandle
_BuildPrimvarDependencies(HdContainerDataSourceHandle const &primSource)
{
    TRACE_FUNCTION();

    const auto compPrimvars =
        HdExtComputationPrimvarsSchema::GetFromParent(primSource);

    if (!compPrimvars) {
        // No ext computation primvars. But they could be added later.
        // So add exactly the dependencies for the dependencies.
        static HdContainerDataSourceHandle const result =
            HdRetainedContainerDataSource::New(
                _tokens->extComputationPrimvarsDependenciesDependency,
                _ExtComputationPrimvarsDependenciesDependency());
        return result;
    }

    TfTokenVector names;
    std::vector<HdDataSourceBaseHandle> sources;

    // The value of the primvar depends on the value of the computation.
    for (const TfToken &name : compPrimvars.GetExtComputationPrimvarNames()) {
        const HdExtComputationPrimvarSchema compPrimvar =
            compPrimvars.GetExtComputationPrimvar(name);

        HdPathDataSourceHandle const sourceComputation =
            compPrimvar.GetSourceComputation();
        if (!sourceComputation) {
            continue;
        }
        HdTokenDataSourceHandle const sourceComputationOutputName =
            compPrimvar.GetSourceComputationOutputName();
        if (!sourceComputationOutputName) {
            continue;
        }

        names.push_back(
            TfToken(
                _tokens->primvarExtComputationDependency.GetString() +
                name.GetString()));

        sources.push_back(
            HdDependencySchema::Builder()
                // Ext computation prim driving this primvar.
                .SetDependedOnPrimPath(
                    compPrimvar.GetSourceComputation())
                // Value of computation output.
                //
                // Similar to above, note that the locator does not correspond
                // to an actual data source in the scene index.
                .SetDependedOnDataSourceLocator(
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdExtComputationSchema::GetOutputsLocator()
                            .Append(
                                sourceComputationOutputName->GetTypedValue(0.0f))
                            .Append(_tokens->value)))
                // Primvar value.
                .SetAffectedDataSourceLocator(
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdPrimvarsSchema::GetDefaultLocator()
                            .Append(name)
                            .Append(HdPrimvarSchemaTokens->primvarValue)))
                .Build());
    }

    // Dependencies for the dependencies.
    names.push_back(_tokens->extComputationPrimvarsDependenciesDependency);
    sources.push_back(_ExtComputationPrimvarsDependenciesDependency());

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), sources.data());
}

// Build dependencies schema.
HdContainerDataSourceHandle
_BuildDependencies(const HdSceneIndexPrim &prim)
{
    TRACE_FUNCTION();

    if (prim.primType == HdPrimTypeTokens->extComputation) {
        return _BuildExtComputationDependencies(prim.dataSource);
    } else {
        return _BuildPrimvarDependencies(prim.dataSource);
    }
}

} // anonymous namespace

/* static */
HdsiExtComputationDependencySceneIndexRefPtr
HdsiExtComputationDependencySceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiExtComputationDependencySceneIndex(inputSceneIndex));
}


HdsiExtComputationDependencySceneIndex::
    HdsiExtComputationDependencySceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdsiExtComputationDependencySceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.dataSource) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            HdRetainedContainerDataSource::New(
                HdDependenciesSchema::GetSchemaToken(),
                _BuildDependencies(prim)),
            prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdsiExtComputationDependencySceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiExtComputationDependencySceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiExtComputationDependencySceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiExtComputationDependencySceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
