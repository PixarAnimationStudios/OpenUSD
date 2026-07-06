//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/particleFieldConversionSceneIndex.h"

#include "pxr/usd/usdVol/tokens.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

const static SdfPath __DefaultParticleFieldMaterial 
    = SdfPath::AbsoluteRootPath().AppendChild(TfToken("__DefaultParticleFieldMaterial"));

TF_DEFINE_PUBLIC_TOKENS(HdsiParticleFieldConversionTokens, HDSI_PARTICLE_FIELD_CONVERSION_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (varname)
    (result)
    (UsdPrimvarReader_float3)
    (diffuseColor)
    (emissiveColor)
    (UsdPreviewSurface)
);

// Width DataSource. Averages particle field scales to approximate a good 
// width value.

class _ParticleWidthValueDataSource final 
    : public HdTypedSampledDataSource<VtFloatArray>
{
public:
    HD_DECLARE_DATASOURCE(_ParticleWidthValueDataSource);

    _ParticleWidthValueDataSource(const HdSampledDataSourceHandle& scalesInput)
    : _scalesInput(scalesInput)
    {
    }

    VtFloatArray GetTypedValue(Time shutterOffset) override
    {
        VtValue value = _scalesInput->GetValue(shutterOffset);
        if (value.IsHolding<VtVec3fArray>() || value.CanCast<VtVec3fArray>()) {
            value = value.Cast<VtVec3fArray>();
            VtVec3fArray scales = value.UncheckedGet<VtVec3fArray>();
            VtFloatArray width(scales.size());
            for (size_t i = 0; i < width.size(); i++)
                width[i] = (scales[i][0] + scales[i][1] + scales[i][2]) * 0.3333f;
            return width;
        }
        return VtFloatArray();
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _scalesInput->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    HdSampledDataSourceHandle _scalesInput;
};

class _ParticleWidthDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParticleWidthDataSource);

    _ParticleWidthDataSource(const HdContainerDataSourceHandle& scalesInput)
    : _scalesInput(scalesInput)
    {
    }

    TfTokenVector
    GetNames() override
    {
        if (!_scalesInput) return TfTokenVector();
        return _scalesInput->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (!_scalesInput) return nullptr;
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _ParticleWidthValueDataSource::New(
                HdSampledDataSource::Cast(_scalesInput->Get(name))
            );
        }
        return _scalesInput->Get(name);
    }

private:
    HdContainerDataSourceHandle _scalesInput;
};

// Points DataSource. Convert positions data source to float typed points.

class _ParticlePointsValueDataSource final 
    : public HdTypedSampledDataSource<VtVec3fArray>
{
public:
    HD_DECLARE_DATASOURCE(_ParticlePointsValueDataSource);

    _ParticlePointsValueDataSource(const HdSampledDataSourceHandle& input)
    : _input(input)
    {
    }

    VtVec3fArray GetTypedValue(Time shutterOffset) override
    {
        VtValue value = _input->GetValue(shutterOffset);
        if (value.IsHolding<VtVec3fArray>() || value.CanCast<VtVec3fArray>()) {
            value = value.Cast<VtVec3fArray>();
            return value.UncheckedGet<VtVec3fArray>();
        }
        return VtVec3fArray();
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _input->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    HdSampledDataSourceHandle _input;
};

class _ParticlePointsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParticlePointsDataSource);

    _ParticlePointsDataSource(const HdContainerDataSourceHandle& input)
    : _input(input)
    {
    }

    TfTokenVector
    GetNames() override
    {
        if (!_input) return TfTokenVector();
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (!_input) return nullptr;
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _ParticlePointsValueDataSource::New(
                HdSampledDataSource::Cast(_input->Get(name))
            );
        }
        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
};

// Coefficients DataSource. Split coefficients data source into seperate each band 
// so they fit within the points vertex primar size. 

class _ParticleCoefficientsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParticleCoefficientsDataSource);

    _ParticleCoefficientsDataSource(
        const HdContainerDataSourceHandle& parent,
        const size_t index, 
        const int degree)
    : _input(HdContainerDataSource::Cast(
            parent->Get(UsdVolTokens->radianceSphericalHarmonicsCoefficients)
        )), 
      _index(index), 
      _stride((degree + 1) * (degree + 1))
    {
    }

    TfTokenVector
    GetNames() override
    {
        if (!_input) return TfTokenVector();
        return _input->GetNames();
    }

    // Take the original coefficients values and copy only the specific indexes
    // we need.
    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input) return nullptr;
        if (name == HdPrimvarSchemaTokens->primvarValue) {

            HdDataSourceBaseHandle inputDs = _input->Get(
                HdPrimvarSchemaTokens->primvarValue);
            if (_stride == 1 && _index == 0) {
                return inputDs;
            }

            if (HdSampledDataSourceHandle coefficientsValueDs
                = HdSampledDataSource::Cast(inputDs)) {
                const VtValue coefficientsValue
                    = coefficientsValueDs->GetValue(0);
                if (coefficientsValue.IsHolding<VtVec3fArray>()) {
                    VtVec3fArray coefficients
                        = coefficientsValue.UncheckedGet<VtVec3fArray>();
                    VtVec3fArray reducedCoefficients(
                        coefficients.size() / _stride);
                    for (size_t i = 0, j = _index;
                         i < reducedCoefficients.size(); i++, j += _stride)
                        reducedCoefficients[i] = coefficients[j];
                    return HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        reducedCoefficients);
                }
                else if (coefficientsValue.IsHolding<VtVec3hArray>()) {
                    VtVec3hArray coefficients
                        = coefficientsValue.UncheckedGet<VtVec3hArray>();
                    VtVec3hArray reducedCoefficients(
                        coefficients.size() / _stride);
                    for (size_t i = 0, j = _index;
                         i < reducedCoefficients.size(); i++, j += _stride)
                        reducedCoefficients[i] = coefficients[j];
                    return HdRetainedTypedSampledDataSource<VtVec3hArray>::New(
                        reducedCoefficients);
                }
                else if (coefficientsValue.IsHolding<VtVec3dArray>()) {
                    VtVec3dArray coefficients
                        = coefficientsValue.UncheckedGet<VtVec3dArray>();
                    VtVec3dArray reducedCoefficients(
                        coefficients.size() / _stride);
                    for (size_t i = 0, j = _index;
                         i < reducedCoefficients.size(); i++, j += _stride)
                        reducedCoefficients[i] = coefficients[j];
                    return HdRetainedTypedSampledDataSource<VtVec3dArray>::New(
                        reducedCoefficients);
                }
            }
            return nullptr;
        }
        else if (name == HdPrimvarSchemaTokens->role) {
            return HdPrimvarSchema::BuildRoleDataSource(
                HdPrimvarRoleTokens->color);
        }
        else if (name == HdPrimvarSchemaTokens->interpolation) {
            return HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->vertex);
        }

        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    size_t _index, _stride;
};

/// Primvars DataSource. Resolve primvars needed for conversion.

class _ParticlePrimvarsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParticlePrimvarsDataSource);

    _ParticlePrimvarsDataSource(
        const HdContainerDataSourceHandle& input,
        const HdSampledDataSourceHandle& constantWidth)
    : _input(input),
      _constantWidth(constantWidth)
    {
    }

    int 
    GetCoefficientsDegree()
    {
        if (!_input) return 0;

        int degree = 0;
        if (HdSampledDataSourceHandle degreeDs = HdPrimvarSchema(
            HdContainerDataSource::Cast(_input->Get(
                UsdVolTokens->radianceSphericalHarmonicsDegree)
            )).GetPrimvarValue()
        ) {
            const VtValue degreeValue = degreeDs->GetValue(0);
            if (degreeValue.IsHolding<int>()) {
                degree = degreeValue.UncheckedGet<int>();
            }
        }

        return degree;
    }

    TfTokenVector
    GetNames() override
    {
        if (!_input) return TfTokenVector();
        TfTokenVector names = _input->GetNames();
        int degree = 0;
        for (TfToken& name : names) {
            // Rename positions as points
            if (name == UsdVolTokens->positions) {
                name = HdTokens->points;
            }
            // Replace spherical harmonics with seperated primvars.
            else if (
                name == UsdVolTokens->radianceSphericalHarmonicsCoefficients
            ) {
                name = HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients00;
                degree = GetCoefficientsDegree();
            }
        }
        // Add additional seperated spherical harmonics as needed by the degree.
        if (degree > 0) {
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients10);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients11);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients12);
        }
        if (degree > 1) {
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients20);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients21);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients22);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients23);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients24);
        }
        if (degree > 2) {
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients30);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients31);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients32);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients33);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients34);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients35);
            names.push_back(
                HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients36);
        }
        names.push_back(HdTokens->widths);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (!_input) return nullptr;
        // Replace our positions with float points
        if (name == HdTokens->points) {
            return _ParticlePointsDataSource::New(
                HdContainerDataSource::Cast(_input->Get(UsdVolTokens->positions)));
        }
        // Add a width primvar
        else if (name == HdTokens->widths) {
            // Use the user provided constant width
            if (_constantWidth) {
                return HdPrimvarSchema::Builder()
                    .SetPrimvarValue(_constantWidth)
                    .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->constant))
                .Build();
            }
            // Otherwise average our scales if they exist
            else if (HdContainerDataSourceHandle scales 
                = HdContainerDataSource::Cast(_input->Get(UsdVolTokens->scales))) {
                return _ParticleWidthDataSource::New(scales);
            }
            // Otherwise set all points size to 1.0
            else {
                return HdPrimvarSchema::Builder()
                    .SetPrimvarValue(HdRetainedTypedSampledDataSource<float>::New(1.f))
                    .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->constant))
                    .Build();
            }
        }
        // Return our seperated spherical harmonics
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients00
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 0, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients10
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 1, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients11
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 2, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients12
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 3, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients20
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 4, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients21
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 5, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients22
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 6, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients23
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 7, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients24
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 8, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients30
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 9, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients31
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 10, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients32
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 11, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients33
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 12, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients34
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 13, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients35
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 14, GetCoefficientsDegree());
        }
        else if (name == 
            HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients36
        ) {
            return _ParticleCoefficientsDataSource::New(
                _input, 15, GetCoefficientsDegree());
        }

        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    HdSampledDataSourceHandle _constantWidth;
};

class _ParticlePrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ParticlePrimDataSource);

    _ParticlePrimDataSource(
        const HdContainerDataSourceHandle& input,
        const HdSampledDataSourceHandle& constantWidth)
    : _input(input),
      _constantWidth(constantWidth)
    {
    }

    TfTokenVector
    GetNames() override
    {
        if (!_input) return TfTokenVector();
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (!_input) return nullptr;
        HdDataSourceBaseHandle result = _input->Get(name);
        if (name == HdPrimvarsSchema::GetSchemaToken()) {
            return _ParticlePrimvarsDataSource::New(
                HdContainerDataSource::Cast(result), _constantWidth
            );
        }
        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    HdSampledDataSourceHandle _constantWidth;
};

/// Particle Conversion Scene Index

HdsiParticleFieldConversionSceneIndexRefPtr
HdsiParticleFieldConversionSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex, 
    const HdSampledDataSourceHandle constantWidth, 
    const HdContainerDataSourceHandle geometryOverlay, 
    const HdContainerDataSourceHandle materialOverlay)
{
    return TfCreateRefPtr(new HdsiParticleFieldConversionSceneIndex(
        inputSceneIndex, constantWidth, geometryOverlay, materialOverlay
    ));
}

HdSceneIndexPrim 
HdsiParticleFieldConversionSceneIndex::GetPrim(const SdfPath& primPath) const
{
    // Generate a USD Preview Material incase the user didn't proivde one which
    // reads radiance:SphericalHarmonicsCoefficients00 as the color.
    // This isn't correct but will give us an approximation for previewing.
    if (primPath == __DefaultParticleFieldMaterial) {
        // USD Primar Reader Params
        static const TfToken usdPrimvarReaderParamNames[] = {
            _tokens->varname
        };
        
        static const HdDataSourceBaseHandle usdPrimvarReaderParams[] = {
            HdMaterialNodeParameterSchema::Builder()
                .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdsiParticleFieldConversionTokens->radianceSphericalHarmonicsCoefficients00))
                .Build()
        };

        // USD Primar Reader Node
        static const HdDataSourceBaseHandle usdPrimvarReaderNode 
            = HdMaterialNodeSchema::Builder()
                .SetParameters(
                    HdRetainedContainerDataSource::New(
                        1, 
                        usdPrimvarReaderParamNames,
                        usdPrimvarReaderParams))
                .SetNodeIdentifier(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _tokens->UsdPrimvarReader_float3))
                .Build();

        // USD Preview Surface Connections
        static const TfToken usdPreviewSurfaceConnectionNames[] = {
            _tokens->emissiveColor
        };

        HdDataSourceBaseHandle usdPreviewSurfaceConnections[] = { 
            HdRetainedSmallVectorDataSource::New(
                1,
                std::array<HdDataSourceBaseHandle, 1> {
                    HdMaterialConnectionSchema::Builder()
                        .SetUpstreamNodePath(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                _tokens->UsdPrimvarReader_float3))
                        .SetUpstreamNodeOutputName(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                _tokens->result))
                        .Build() }
                    .data()) 
        };

        // USD Preview Surface Params
        static const TfToken usdPreviewSurfaceParamNames[] = {
            _tokens->diffuseColor
        };
        
        static const HdDataSourceBaseHandle usdPreviewSurfaceParams[] = {
            HdMaterialNodeParameterSchema::Builder()
                .SetValue(HdRetainedTypedSampledDataSource<VtValue>::New(
                    VtValue(GfVec3f(0.f, 0.f, 0.f))))
                .Build()
        };

        // USD Preview Surface Node
        static const HdDataSourceBaseHandle usdPreviewSurfaceNode 
            = HdMaterialNodeSchema::Builder()
                .SetParameters(
                    HdRetainedContainerDataSource::New(
                        1, 
                        usdPreviewSurfaceParamNames,
                        usdPreviewSurfaceParams))
                .SetInputConnections(
                    HdRetainedContainerDataSource::New(
                        1, 
                        usdPreviewSurfaceConnectionNames,
                        usdPreviewSurfaceConnections))
                .SetNodeIdentifier(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _tokens->UsdPreviewSurface))
                .Build();

        // USD Preview Surface Nodes
        static const TfToken usdPreviewSurfaceNodeNames[] = { 
            _tokens->UsdPrimvarReader_float3, 
            _tokens->UsdPreviewSurface 
        };
        static const HdDataSourceBaseHandle usdPreviewSurfaceNodes[] = { 
            usdPrimvarReaderNode, 
            usdPreviewSurfaceNode 
        };

        static const HdContainerDataSourceHandle nodesDs 
            = HdRetainedContainerDataSource::New(
                2,
                usdPreviewSurfaceNodeNames,
                usdPreviewSurfaceNodes
        );

        // USD Preview Surface Terminals
        static const HdContainerDataSourceHandle terminalsDs 
            = HdRetainedContainerDataSource::New(
                HdMaterialTerminalTokens->surface,
                HdMaterialConnectionSchema::Builder()
                    .SetUpstreamNodePath(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _tokens->UsdPreviewSurface))
                    .SetUpstreamNodeOutputName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMaterialTerminalTokens->surface))
                    .Build()
        );

        // USD Preview Surface Material Network
        static const HdDataSourceBaseHandle materialNetworkDs 
            = HdMaterialNetworkSchema::Builder()
                .SetNodes(nodesDs)
                .SetTerminals(terminalsDs)
                .Build();

        // USD Preview Surface Material
        static const HdContainerDataSourceHandle materialDS 
            = HdMaterialSchema::BuildRetained(
                1, 
                &HdMaterialSchemaTokens->universalRenderContext,
                &materialNetworkDs
        );

        // Return our USD Preview Surface with our optional input material
        // overlayed ontop.
        return {
            HdPrimTypeTokens->material,
            HdOverlayContainerDataSource::New(
                _materialOverlay,
                HdRetainedContainerDataSource::New(
                    HdMaterialSchema::GetSchemaToken(), materialDS)
            )
        };
    }

    // Look for particle fields and replace them with our point conversion.
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->particleField) {

        // Bind our default material and overlay under our input. 
        // If the user bound a material that should take precedence.
        static const TfToken purposes[] = {
            HdMaterialBindingsSchemaTokens->allPurpose
        };
        static const HdDataSourceBaseHandle materialBindingSources[] = {
            HdMaterialBindingSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    __DefaultParticleFieldMaterial))
        };

        // Wrap our input in _ParticlePrimDataSource to convert 
        // from particles to points. We overlay our material binding and optional
        // geometryOverlay input.
        return {
            HdPrimTypeTokens->points,
            HdOverlayContainerDataSource::New(
                _ParticlePrimDataSource::New(
                    HdContainerDataSource::Cast(prim.dataSource), _constantWidth),
                HdRetainedContainerDataSource::New(
                    HdMaterialBindingsSchemaTokens->materialBindings, 
                    HdMaterialBindingsSchema::BuildRetained(1, purposes, materialBindingSources)
                ),
                _geometryOverlay
            )
        };
    }
    return prim;
}

SdfPathVector
HdsiParticleFieldConversionSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    SdfPathVector children = _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    if (primPath == SdfPath::AbsoluteRootPath()) {
        // Add our default material under the root.
        children.push_back(__DefaultParticleFieldMaterial);
    }
    return children;
}

HdsiParticleFieldConversionSceneIndex::HdsiParticleFieldConversionSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdSampledDataSourceHandle constantWidth, 
    const HdContainerDataSourceHandle geometryOverlay, 
    const HdContainerDataSourceHandle materialOverlay)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex),
        _constantWidth(constantWidth),
        _geometryOverlay(geometryOverlay),
        _materialOverlay(materialOverlay)
{
}

void 
HdsiParticleFieldConversionSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries added;
    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        if (entry.primType == HdPrimTypeTokens->particleField) {
            added.push_back({ entry.primPath, HdPrimTypeTokens->points });
        }
        else if (entry.primPath == SdfPath::AbsoluteRootPath()) {
            // Add our default material when the root is added.
            added.push_back({ 
                __DefaultParticleFieldMaterial, 
                HdPrimTypeTokens->material 
            });
            added.push_back(entry);
        }
        else {
            added.push_back(entry);
        }
    }
    _SendPrimsAdded(added);
}

void 
HdsiParticleFieldConversionSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void 
HdsiParticleFieldConversionSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
