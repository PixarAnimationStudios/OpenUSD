//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/updateObjectSettingsSceneIndex.h"

#if PXR_VERSION >= 2308

#include "hdPrman/debugCodes.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/tokens.h"

#if PXR_VERSION >= 2311
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#elif PXR_VERSION == 2308
#include "pxr/imaging/hd/modelSchema.h"
#endif

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((riAttributesDiceMicropolyonLength,
        "ri:attributes:dice:micropolygonlength"))
    ((riAttributesDiceRasterOrient,
        "ri:attributes:dice:rasterorient"))
    ((riAttributesShadeShadingRate,
        "ri:attributes:shade:shadingrate"))
    ((riAttributesTraceDisplacements,
        "ri:attributes:trace:displacements"))
);


static HdDataSourceBaseHandle
_UpdatePrimvars(HdPrimvarsSchema primvars)
{
    HdContainerDataSourceEditor primvarsEditor(primvars.GetContainer());

    // boolean attributes that migrated from string to int
    // XXX array loop
    if (HdPrimvarSchema primvar  =
        primvars.GetPrimvar(_tokens->riAttributesDiceRasterOrient)) {
        if (HdStringDataSourceHandle strDs =
            HdStringDataSource::Cast(primvar.GetPrimvarValue())) {
            std::string strVal = strDs->GetTypedValue(0.0f);
            bool boolVal = (strVal == "Yes");
            // Overlay boolean value
            primvarsEditor.Overlay(
                HdDataSourceLocator(
                    _tokens->riAttributesDiceRasterOrient),
                HdPrimvarSchema::Builder()
                    .SetPrimvarValue(
                        HdRetainedTypedSampledDataSource<bool>::New(boolVal))
                    .Build());
        }
    }

    // shade:shadingrate -> dice:micropolygonlength
    if (HdPrimvarSchema shadingRate =
        primvars.GetPrimvar(_tokens->riAttributesShadeShadingRate)) {
        if (primvars.GetPrimvar(_tokens->riAttributesDiceMicropolyonLength)) {
            // If micropolygonlength is already set, leave as-is.
        } else {
            // Convert shadingrate to micropolygonlength
            if (HdFloatArrayDataSourceHandle rateDs =
                HdFloatArrayDataSource::Cast(shadingRate.GetPrimvarValue())) {
                VtArray<float> rateVal = rateDs->GetTypedValue(0.0f);
                if (rateVal.size() == 2) {
                    // Use 1st component only
                    float shadingRate_0 = rateVal[0];
                    // micropolygonlength = sqrt(shadingRate[0])
                    float micropolygonLength = sqrt(shadingRate_0);
                    primvarsEditor.Overlay(
                        HdDataSourceLocator(
                            _tokens->riAttributesDiceMicropolyonLength),
                        HdPrimvarSchema::Builder()
                            .SetPrimvarValue(
                              HdRetainedTypedSampledDataSource<float>::New(
                                micropolygonLength))
                            .SetInterpolation(
                                HdPrimvarSchema::BuildInterpolationDataSource(
                                    HdPrimvarSchemaTokens->constant))
                            .Build());
                }
            }
        }
        // Erase shadingrate
        primvarsEditor.Set(
            HdDataSourceLocator(_tokens->riAttributesShadeShadingRate),
            HdBlockDataSource::New());
    }

    // trace:displacements:
    // Values above 1 are deprecated, and are now equivalent to 1.
    if (HdPrimvarSchema traceDisplacements =
        primvars.GetPrimvar(_tokens->riAttributesTraceDisplacements)) {
        if (HdIntDataSourceHandle intDs =
            HdIntDataSource::Cast(traceDisplacements.GetPrimvarValue())) {
            if (intDs->GetTypedValue(0) > 1) {
                // Overlay a new value of 1
                primvarsEditor.Overlay(
                    HdDataSourceLocator(
                        _tokens->riAttributesTraceDisplacements),
                    HdPrimvarSchema::Builder()
                        .SetPrimvarValue(
                            HdRetainedTypedSampledDataSource<int>::New(1))
                        .Build());
            }
        }
    }

    return primvarsEditor.Finish();
}

/// This data source returns a custom "primvars" data source when queried
class HdPrman_UpdateObjectSettings_DataSource
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdPrman_UpdateObjectSettings_DataSource);

    TfTokenVector
    GetNames() override
    {
        TfTokenVector result;
        if (_inputPrimDs) {
            result = _inputPrimDs->GetNames();
        }
        return result;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result;
        if (_inputPrimDs) {
            result = _inputPrimDs->Get(name);
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            if (HdPrimvarsSchema primvars =
                HdPrimvarsSchema::GetFromParent(_inputPrimDs)) {
                result = _UpdatePrimvars(primvars);
            }
        }
        return result;
    }

protected:
    HdPrman_UpdateObjectSettings_DataSource(
        HdContainerDataSourceHandle const& inputDs)
    : _inputPrimDs(inputDs)
    {
    }

private:
    HdContainerDataSourceHandle _inputPrimDs;
};



/* static */
HdPrman_UpdateObjectSettingsSceneIndexRefPtr
HdPrman_UpdateObjectSettingsSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(  
        new HdPrman_UpdateObjectSettingsSceneIndex(inputSceneIndex));
}

HdPrman_UpdateObjectSettingsSceneIndex::HdPrman_UpdateObjectSettingsSceneIndex(
                                                                             const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{ }

HdSceneIndexPrim 
HdPrman_UpdateObjectSettingsSceneIndex::GetPrim(
                                               const SdfPath &primPath) const
{
    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    return {
        prim.primType,
        HdPrman_UpdateObjectSettings_DataSource::New(prim.dataSource)
    };
}

SdfPathVector 
HdPrman_UpdateObjectSettingsSceneIndex::GetChildPrimPaths(
                                                         const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdPrman_UpdateObjectSettingsSceneIndex::_PrimsAdded(
                                                   const HdSceneIndexBase &sender,
                                                   const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    _SendPrimsAdded(entries);
}

void 
HdPrman_UpdateObjectSettingsSceneIndex::_PrimsRemoved(
                                                     const HdSceneIndexBase &sender,
                                                     const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdPrman_UpdateObjectSettingsSceneIndex::_PrimsDirtied(
                                                     const HdSceneIndexBase &sender,
                                                     const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // XXX transfer primvar dirtiness
    _SendPrimsDirtied(entries);
}

HdPrman_UpdateObjectSettingsSceneIndex::~HdPrman_UpdateObjectSettingsSceneIndex()
= default;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
