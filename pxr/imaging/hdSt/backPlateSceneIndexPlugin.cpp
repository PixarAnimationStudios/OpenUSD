// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "backPlateSceneIndexPlugin.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/schemaTypeDefs.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hdsi/backPlateSceneIndex.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_BackPlateSceneIndexPlugin"))
);

TF_DEFINE_PRIVATE_TOKENS(_materialNodeNamesTokens,
    (UsdPrimvarReader_float2)
    (UsdUVTexture)
    (DepthTexture)
    (UnlitSurface)
);

TF_DEFINE_PRIVATE_TOKENS(_backPlateAttrTokens,
    (image)
    (alphaImage)
    (depthImage)
    (power)
    (slope)
    (offset)
    (depthMinOffset)
    (depthNormalizingFactor)
    (depthCameraSpaceOffset)
);

TF_DEFINE_PRIVATE_TOKENS(_materialNodeParamsTokens,
    (rgb)
    (r)
    (displacement)
    (a)
    (clamp)
    (opacity)
    (file)
    (st)
    (wrapS)
    (wrapT)
    (material)
    (implementationSource)
    (backPlateColor)
    (sourceAsset)
    (varname)
    (result)
    ((glslfxSourceAsset, "glslfx:sourceAsset"))
    ((glslfxShader, "shaders/backPlateSurface.glslfx"))
);

namespace {
static const char* const _pluginDisplayName = "GL";
static constexpr int _depthPlateSubdivs = 32;
static constexpr int _flatPlateSubdivs = 1;
static constexpr int _plateRefineLevel = 3;
}

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_BackPlateSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName, _tokens->sceneIndexPluginName, nullptr,
        insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdSt_BackPlateSceneIndexPlugin::HdSt_BackPlateSceneIndexPlugin() = default;

///////////////////////// HdSt BackPlateSceneIndex /////////////////////////////
namespace {
class HdSt_BackPlateSceneIndex : public HdsiBackPlateSceneIndex
{
public:
    static HdsiBackPlateSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new HdSt_BackPlateSceneIndex(inputSceneIndex));
    }

    using HdsiBackPlateSceneIndex::_GetBackPlateSchema;
    using HdsiBackPlateSceneIndex::_GetDefaultTypedValue;
    using HdsiBackPlateSceneIndex::_ComputeEffectiveDimensions;
    using HdsiBackPlateSceneIndex::_ComputeLocalXform; 

protected:
    HdSt_BackPlateSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
        : HdsiBackPlateSceneIndex(inputSceneIndex) {}

    HdContainerDataSourceHandle
    _BuildMaterialDataSource(
        const TfToken &instanceName,
        const HdContainerDataSourceHandle &primDS) const override;
    
    HdContainerDataSourceHandle
    _BuildMeshPrimDataSource(
        const SdfPath &platePath,
        const TfToken &instanceName,
        const HdContainerDataSourceHandle &primDS) const override;

    HdDataSourceLocatorSet
    _GetAdditionalDirtyLocators() const override;
};

/////////////////////////// DataSource Builders ///////////////////////////////

int _GetPlateSubdiv(bool hasDepth) 
{
    return hasDepth ? _depthPlateSubdivs : _flatPlateSubdivs; 
}

class _AssetPathDataSource : public HdAssetPathDataSource 
{
public:
    HD_DECLARE_DATASOURCE(_AssetPathDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    SdfAssetPath GetTypedValue(Time shutterOffset) override {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        SdfAssetPath assetPath;
        if (!HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
                _GetAssetType(backPlate), shutterOffset, &assetPath)){
            return SdfAssetPath();
        };
        return assetPath;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        HdSampledDataSourceHandle source = _GetAssetType(backPlate);
        if (!source) {
            return false;
        }
        return source->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _AssetPathDataSource(TfToken const &instanceName,
                        TfToken const & assetType,
                        HdContainerDataSourceHandle const &primDS)
    : _instanceName(instanceName)
    , _assetType(assetType)
    , _primDS(primDS){}

    HdAssetPathDataSourceHandle 
    _GetAssetType(HdBackPlateSchema const &backPlate) const {
        if (_assetType == _backPlateAttrTokens->image) {
            return backPlate.GetImage();
        }
        if (_assetType == _backPlateAttrTokens->depthImage) {
            return backPlate.GetDepthImage();
        }
        if (_assetType == _backPlateAttrTokens->alphaImage) {
            return backPlate.GetAlphaImage();
        }
        return nullptr;
    }

    TfToken const _instanceName;
    TfToken const _assetType;
    HdContainerDataSourceHandle const _primDS;
};

class _DepthVarDataSource : public HdFloatDataSource 
{
public:
    HD_DECLARE_DATASOURCE(_DepthVarDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    float GetTypedValue(Time shutterOffset) override {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        float result(0.0);
        // if _type is depthNormalizingFactor we change the default to 1.0.
        if (_type == _backPlateAttrTokens->depthNormalizingFactor) {
            result = 1.0;
        }

        if (!HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
                _GetVarType(backPlate), shutterOffset, &result)) {
            return result;
        }
        return result;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        HdSampledDataSourceHandle source = _GetVarType(backPlate);
        if (!source) {
            return false;
        }
        return source->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _DepthVarDataSource(TfToken const &instanceName,
                        TfToken const & type,
                        HdContainerDataSourceHandle const &primDS)
    : _instanceName(instanceName)
    , _type(type)
    , _primDS(primDS){}

    HdFloatDataSourceHandle 
    _GetVarType(HdBackPlateSchema const &backPlate) const {
        if (_type == _backPlateAttrTokens->depthMinOffset) {
            return backPlate.GetDepthMinOffset();
        }
        if (_type == _backPlateAttrTokens->depthNormalizingFactor) {
            return backPlate.GetDepthNormalizingFactor();
        }
        if (_type == _backPlateAttrTokens->depthCameraSpaceOffset) {
            return backPlate.GetDepthCameraSpaceOffset();
        }
        
        return nullptr;
    }

    TfToken const _instanceName;
    TfToken const _type;
    HdContainerDataSourceHandle const _primDS;
};

class _ColorGradingDataSource : public HdVec3fDataSource 
{
public:
    HD_DECLARE_DATASOURCE(_ColorGradingDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    GfVec3f GetTypedValue(Time shutterOffset) override {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        GfVec3f result(0.0);
        // if _type is offset we leave the default as (0, 0, 0).
        if (_type == _backPlateAttrTokens->slope || 
            _type == _backPlateAttrTokens->power) {
            result = GfVec3f(1.0);
        }

        if (!HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
                _GetLumaSource(backPlate), shutterOffset, &result)) {
            return result;
        }
        return result;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        HdSampledDataSourceHandle source = _GetLumaSource(backPlate);
        if (!source) {
            return false;
        }
        return source->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _ColorGradingDataSource(TfToken const &instanceName,
                            TfToken const & type,
                            HdContainerDataSourceHandle const &primDS)
    : _instanceName(instanceName)
    , _type(type)
    , _primDS(primDS){}

    HdVec3fDataSourceHandle 
    _GetLumaSource(HdBackPlateSchema const &backPlate) const {
        if (_type == _backPlateAttrTokens->offset) {
            return backPlate.GetLumaOffset();
        }
        if (_type == _backPlateAttrTokens->slope) {
            return backPlate.GetLumaSlope();
        }
        if (_type == _backPlateAttrTokens->power) {
            return backPlate.GetLumaPower();
        }
        return nullptr;
    }

    TfToken const _instanceName;
    TfToken const _type;
    HdContainerDataSourceHandle const _primDS;
};

class _PointsValueDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsValueDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    VtVec3fArray GetTypedValue(Time t) override {
        GfVec2f dims = HdSt_BackPlateSceneIndex::_ComputeEffectiveDimensions( 
                            _instanceName, _primDS, t);
        float width = dims[0];
        float length = dims[1];
        const int N = _GetPlateSubdiv(_hasDepth); 
        const int dim  = N + 1;
        VtVec3fArray pts(dim*dim);
        for (int r = 0; r < dim; ++r) {
            for (int i = 0; i < dim; ++i) {
                const float u = float(i) / N;
                const float v = float(r) / N;
                pts[r*dim + i] = 
                    GfVec3f((u - 0.5f) * width, (v - 0.5f) * length, 0.0f);
            }
        }
        return pts;
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        HdCameraSchema cam = HdCameraSchema::GetFromParent(_primDS);
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        HdSampledDataSourceHandle sources[] = {
            cam.GetHorizontalAperture(),
            cam.GetVerticalAperture(),
            cam.GetFocalLength(),
            cam.GetFocusDistance(),
            cam.GetProjection(),
            backPlate.GetScaleTweak()
        };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources),
            sources, 
            startTime, 
            endTime, 
            outSampleTimes);
    }

private:
    _PointsValueDataSource(TfToken const &instanceName,
                        HdContainerDataSourceHandle const &primDS,
                        bool const hasDepth)
    : _instanceName(instanceName)
    , _primDS(primDS)
    , _hasDepth(hasDepth) {}

    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDS;
    bool const _hasDepth;
}; 

class _XformMatrixDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_XformMatrixDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    GfMatrix4d GetTypedValue(Time shutterOffset) override {
        HdXformSchema camXform = HdXformSchema::GetFromParent(
            _primDS);

        const GfMatrix4d local = 
            HdSt_BackPlateSceneIndex::_ComputeLocalXform(
                _primDS, _instanceName, shutterOffset);
        GfMatrix4d camXformMatrix(1.0);

        if (camXform) {
            HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
                camXform.GetMatrix(), shutterOffset, &camXformMatrix);
        }
        return local * camXformMatrix;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdCameraSchema cam = 
            HdCameraSchema::GetFromParent(_primDS);
        HdXformSchema camXform = 
            HdXformSchema::GetFromParent(_primDS);
        HdBackPlateSchema backPlate = 
            HdSt_BackPlateSceneIndex::_GetBackPlateSchema(_instanceName, 
                                                         _primDS);
        HdSampledDataSourceHandle sources[] = {
            cam.GetFocusDistance(),
            backPlate.GetTranslateTweak(),
            backPlate.GetRotateXYZTweak(),
            camXform.GetMatrix()
        };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), 
            sources, 
            startTime, 
            endTime, 
            outSampleTimes);
    }

private:
    _XformMatrixDataSource(TfToken const &instanceName,
                            HdContainerDataSourceHandle const &primDS)
    : _instanceName(instanceName)
    , _primDS(primDS) {}

    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDS;
};

class _PrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result
            {HdPrimvarsSchemaTokens->points, 
            _materialNodeParamsTokens->st};
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            HdContainerDataSourceHandle pointsPrimvarDs =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    _PointsValueDataSource::New(
                        _instanceName, _primDS, _hasDepth))
                .SetInterpolation(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrimvarSchemaTokens->vertex))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrimvarSchemaTokens->point))
                .Build();
            return pointsPrimvarDs;
        }
        if (name == _materialNodeParamsTokens->st) {
            const int N = _GetPlateSubdiv(_hasDepth); 
            const int dim = N+1;
            VtVec2fArray st(dim*dim);
            for (int r = 0; r < dim; ++r) {
                for (int i = 0; i < dim; ++i) {
                    st[r*dim + i] = GfVec2f(float(i)/N, float(r)/N);
                }
            }

            const HdContainerDataSourceHandle stPrimvarDs =
                HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec2fArray>::New(
                        st))
                .SetInterpolation(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdPrimvarSchemaTokens->vertex))
                .SetRole(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        TfToken()))
                .Build();
            return stPrimvarDs;
        }
        return nullptr;
    }

private:
    _PrimvarsDataSource(
        TfToken const & instanceName,
        HdContainerDataSourceHandle const &primDS,
        bool const &hasDepth)
    : _instanceName(instanceName),
    _primDS(primDS),
    _hasDepth(hasDepth)
    {
    }
    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDS;
    bool const _hasDepth;
};

HdDataSourceBaseHandle
_BuildXformDataSource(
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDS) {

    HdDataSourceBaseHandle xformDs = HdXformSchema::Builder()
        .SetMatrix(
            _XformMatrixDataSource::New(instanceName, primDS))
        .SetResetXformStack(
            HdRetainedTypedSampledDataSource<bool>::New(false))
        .Build();
    return xformDs;
}

class _RefineLevelDataSource : public HdIntDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RefineLevelDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    int GetTypedValue(Time shutterOffset) override {
        HdLegacyDisplayStyleSchema displayStyle = 
            HdLegacyDisplayStyleSchema::GetFromParent(_primDS);
        int refineLevel = 0;
        HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
            displayStyle.GetRefineLevel(), shutterOffset, &refineLevel);
        return refineLevel;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdLegacyDisplayStyleSchema displayStyle = 
            HdLegacyDisplayStyleSchema::GetFromParent(_primDS);
        if (const HdSampledDataSourceHandle &source = 
            displayStyle.GetRefineLevel()) {
            return source->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
        }
        return false;
    }

private:
    _RefineLevelDataSource(HdContainerDataSourceHandle const &primDS)
    : _primDS(primDS) {}

    HdContainerDataSourceHandle const _primDS;
};

HdContainerDataSourceHandle
_BuildMeshTopologyDataSource(const bool hasDepth) 
{
    VtIntArray numVerts;
    VtIntArray verts;
    const int N = _GetPlateSubdiv(hasDepth);
    const int dim = N+1;
    numVerts = VtIntArray(N*N, 4);
    verts.reserve(N * N * 4);
    for (int r = 0; r < N; ++r) {
        for (int i = 0; i < N; ++i) {
            const int botL = r*dim + i;
            const int botR = r*dim + i + 1;
            const int topL = (r+1)*dim + i;
            const int topR = (r+1)*dim + i + 1;
            verts.insert(verts.end(), {topR, topL, botL, botR});
        }
    }

    HdContainerDataSourceHandle topologyDs =
        HdMeshTopologySchema::Builder()
            .SetFaceVertexCounts(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(numVerts))
            .SetFaceVertexIndices(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(verts))
            .SetOrientation(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdMeshTopologySchemaTokens->rightHanded))
            .Build();
    return topologyDs;
}

HdContainerDataSourceHandle
_BuildMaterialBindingsDataSource(const SdfPath &platePath)
{
    SdfPath materialPath =
        platePath.AppendChild(_materialNodeParamsTokens->material);
    HdDataSourceBaseHandle materialBindingDs = 
                HdMaterialBindingSchema::Builder()
                    .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(
                        materialPath))
                    .Build();
    return HdRetainedContainerDataSource::New(
            HdMaterialBindingsSchemaTokens->allPurpose,
            materialBindingDs);
}

HdContainerDataSourceHandle
_BuildMeshDataSource(const bool hasDepth)
{
    return HdRetainedContainerDataSource::New( 
        HdMeshSchemaTokens->topology,
            _BuildMeshTopologyDataSource(hasDepth),
        HdMeshSchemaTokens->subdivisionScheme,
            HdRetainedTypedSampledDataSource<TfToken>::New(
                PxOsdOpenSubdivTokens->catmullClark),
        HdMeshSchemaTokens->doubleSided,
            HdRetainedTypedSampledDataSource<bool>::New(false));
}

HdContainerDataSourceHandle
_BuildDisplayStyleDataSource(
    const HdContainerDataSourceHandle &primDS, bool hasDepth)
{
    if (hasDepth) {
        return HdLegacyDisplayStyleSchema::Builder()
            .SetRefineLevel(
                HdRetainedTypedSampledDataSource<int>::New(
                    _plateRefineLevel))
            .Build();
    }
    return HdLegacyDisplayStyleSchema::Builder()
        .SetRefineLevel(
            _RefineLevelDataSource::New(primDS))
        .Build();
}

HdRetainedSmallVectorDataSourceHandle
_BuildConnection(const TfToken outputNodeName, const TfToken upstreamNodeName) 
{
    return HdRetainedSmallVectorDataSource::New(
        1,
        std::array<HdDataSourceBaseHandle, 1>{
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        outputNodeName))
                .SetUpstreamNodeOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        upstreamNodeName))
                .Build()
        }.data()
    );
}

HdContainerDataSourceHandle
_BuildMaterialNetworkDataSource (
    TfToken const &appliedInstanceName,
    HdContainerDataSourceHandle const &primDS)
{
    // USD Primvar Reader Params
    static const HdDataSourceBaseHandle usdPrimvarReaderParams = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<VtValue>::New(
                VtValue(_materialNodeParamsTokens->st)))
            .Build()};

    // USD Primvar Reader Node
    const HdDataSourceBaseHandle primvarReaderNode
        = HdMaterialNodeSchema::Builder()
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    _materialNodeParamsTokens->varname,
                    usdPrimvarReaderParams))
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    _materialNodeNamesTokens->UsdPrimvarReader_float2))
            .Build();

    /// USD UVTextureNode Params
    static const TfToken texParamNames[] = { 
        _materialNodeParamsTokens->file, 
        _materialNodeParamsTokens->st, 
        _materialNodeParamsTokens->wrapS, 
        _materialNodeParamsTokens->wrapT };
    _AssetPathDataSource::Handle assetPathDs = _AssetPathDataSource::New(
        appliedInstanceName, _backPlateAttrTokens->image, primDS);
    _AssetPathDataSource::Handle depthAssetPathDs = 
        _AssetPathDataSource::New(
            appliedInstanceName, _backPlateAttrTokens->depthImage, primDS);
    HdDataSourceBaseHandle texParams[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(assetPathDs)
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeParamsTokens->st))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeParamsTokens->clamp))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeParamsTokens->clamp))
            .Build()};

    // USD UVTextureNode Connections
    static const TfToken texConnectionNames[] =
        {_materialNodeParamsTokens->st};
    HdDataSourceBaseHandle texConnections[] = {
        _BuildConnection(_materialNodeNamesTokens->UsdPrimvarReader_float2, 
            _materialNodeParamsTokens->result)
    };
    // USD UVTextureNode 
    const HdDataSourceBaseHandle textureNode =
        HdMaterialNodeSchema::Builder()
        .SetNodeIdentifier(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeNamesTokens->UsdUVTexture))
        .SetInputConnections(
            HdRetainedContainerDataSource::New(
                1, texConnectionNames, texConnections))
        .SetParameters(
            HdRetainedContainerDataSource::New(4, texParamNames, texParams))
        .Build();

    static const TfToken depthParamNames[] = { 
        _materialNodeParamsTokens->file, 
        _materialNodeParamsTokens->st
    };

    HdDataSourceBaseHandle depthParams[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(depthAssetPathDs)
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeParamsTokens->st))
            .Build()};
    const HdDataSourceBaseHandle depthTextureNode =
    HdMaterialNodeSchema::Builder()
        .SetNodeIdentifier(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeNamesTokens->UsdUVTexture))
        .SetInputConnections(
            HdRetainedContainerDataSource::New(
                1, texConnectionNames, texConnections))
        .SetParameters(
            HdRetainedContainerDataSource::New(
                2, depthParamNames, depthParams))
        .Build();

    // USD Unlit Surface Connections
    static const HdDataSourceBaseHandle depthConnection = {
        _BuildConnection(
            _materialNodeNamesTokens->DepthTexture,
            _materialNodeParamsTokens->r)
    };
    static const HdDataSourceBaseHandle surfaceConnection = {
        _BuildConnection(
            _materialNodeNamesTokens->UsdUVTexture,
            _materialNodeParamsTokens->rgb)
        };
    static const HdDataSourceBaseHandle opacityConnection = {
        _BuildConnection(
            _materialNodeNamesTokens->UsdUVTexture,
            _materialNodeParamsTokens->a)
    };
    static const TfToken usdUnlitSurfaceConnectionNames[] = {
        _materialNodeParamsTokens->backPlateColor,
        _materialNodeParamsTokens->displacement,
        _materialNodeParamsTokens->opacity
    };
    static const HdDataSourceBaseHandle usdUnlitSurfaceConnections[] = {
        surfaceConnection, depthConnection, opacityConnection
    };
    static const TfToken usdUnlitSurfaceParamNames[] = {
        _backPlateAttrTokens->offset,
        _backPlateAttrTokens->slope,
        _backPlateAttrTokens->power,
        _backPlateAttrTokens->depthMinOffset,
        _backPlateAttrTokens->depthNormalizingFactor,
        _backPlateAttrTokens->depthCameraSpaceOffset,
        _materialNodeParamsTokens->opacity
    };
    const HdDataSourceBaseHandle usdUnlitSurfaceParams[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->offset, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->slope, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->power, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_DepthVarDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->depthMinOffset, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_DepthVarDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->depthNormalizingFactor, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_DepthVarDataSource::New(
                appliedInstanceName, 
                _backPlateAttrTokens->depthCameraSpaceOffset, 
                primDS))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
        .SetValue(HdRetainedTypedSampledDataSource<float>::New(1.0f))
        .Build()};

    const HdContainerDataSourceHandle nodeTypeInfoDs = 
        HdRetainedContainerDataSource::New(
            _materialNodeParamsTokens->implementationSource,
            HdRetainedTypedSampledDataSource<TfToken>::New(
                _materialNodeParamsTokens->sourceAsset),
            _materialNodeParamsTokens->glslfxSourceAsset,
                HdRetainedTypedSampledDataSource<SdfAssetPath>::New(
                    SdfAssetPath(PlugFindPluginResource(PLUG_THIS_PLUGIN, 
                        _materialNodeParamsTokens->glslfxShader))));
    
    // If the asset source contains opacity then we connect that input to the 
    // displayed opacity. If not then we default the opacity to 1 via the 
    // usdUnlitSurfaceParameters.
    bool assetHasOpacity = 
        !assetPathDs->GetTypedValue(0.0).GetAssetPath().empty();
    const int paramNum = assetHasOpacity ? 6 : 7;
    const int connNum = assetHasOpacity ? 3 : 2;

    const HdDataSourceBaseHandle unlitSurfaceNode =
        HdMaterialNodeSchema::Builder()
            .SetNodeTypeInfo(nodeTypeInfoDs)
            .SetParameters(
                HdRetainedContainerDataSource::New(
                paramNum,
                usdUnlitSurfaceParamNames, 
                usdUnlitSurfaceParams)
            )
            .SetInputConnections(HdRetainedContainerDataSource::New(
                connNum,
                usdUnlitSurfaceConnectionNames, 
                usdUnlitSurfaceConnections))
            .Build();
    // USD Unlit Surface Nodes
    const HdContainerDataSourceHandle nodesDs = 
        HdRetainedContainerDataSource::New(
            _materialNodeNamesTokens->UsdPrimvarReader_float2, 
                primvarReaderNode,
            _materialNodeNamesTokens->UnlitSurface, unlitSurfaceNode,
            _materialNodeNamesTokens->DepthTexture, depthTextureNode,
            _materialNodeNamesTokens->UsdUVTexture, textureNode);

    // USD Unlit Surface Terminals
    static const HdContainerDataSourceHandle terminalsDs
        = HdRetainedContainerDataSource::New(
            HdMaterialTerminalTokens->surface,
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _materialNodeNamesTokens->UnlitSurface))
                .SetUpstreamNodeOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdMaterialTerminalTokens->surface))
                .Build()
    );

    // USD Unlit Surface Material Network
    const HdDataSourceBaseHandle materialNetworkDs
        = HdMaterialNetworkSchema::Builder()
            .SetNodes(nodesDs)
            .SetTerminals(terminalsDs)
            .Build();

    // USD Unlit Surface Material
    const HdContainerDataSourceHandle materialDs
        = HdMaterialSchema::BuildRetained(
            1,
            &HdMaterialSchemaTokens->universalRenderContext,
            &materialNetworkDs
    );
    return materialDs;
}

class _MaterialDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MaterialDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdMaterialSchemaTokens->material
        };
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdMaterialSchemaTokens->material) {
            return _BuildMaterialNetworkDataSource(_instanceName,
                                                    _primDS);
        }
        return nullptr;
    }

private:
    _MaterialDataSource(
        TfToken const & instanceName,
        HdContainerDataSourceHandle const &primDS)
    : _instanceName(instanceName),
    _primDS(primDS)
    {}
    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDS;
};

////////////////////////////// HDST Overrides /////////////////////////////////
HdContainerDataSourceHandle
HdSt_BackPlateSceneIndex::_BuildMaterialDataSource(
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDS) const
{
    return _MaterialDataSource::New(instanceName, primDS);
}

HdContainerDataSourceHandle
HdSt_BackPlateSceneIndex::_BuildMeshPrimDataSource(
    const SdfPath &platePath,
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDS) const
{
    HdBackPlateSchema backPlate =
        HdSt_BackPlateSceneIndex::_GetBackPlateSchema(
            instanceName, primDS);
    bool hasDepth = false;
    if (backPlate) {
        SdfAssetPath depthAsset;
        HdSt_BackPlateSceneIndex::_GetDefaultTypedValue(
            backPlate.GetDepthImage(), 0.0, &depthAsset);
        hasDepth = depthAsset != SdfAssetPath();
    }
    return
        HdRetainedContainerDataSource::New(
            HdMeshSchemaTokens->mesh,
                _BuildMeshDataSource(hasDepth),
            HdPrimvarsSchemaTokens->primvars,
                _PrimvarsDataSource::New(instanceName, primDS, hasDepth),
            HdXformSchemaTokens->xform,
                _BuildXformDataSource(instanceName, primDS),
            HdLegacyDisplayStyleSchema::GetSchemaToken(),
                _BuildDisplayStyleDataSource(primDS, hasDepth),
            HdMaterialBindingsSchema::GetSchemaToken(),
                _BuildMaterialBindingsDataSource(platePath));
}

HdDataSourceLocatorSet
HdSt_BackPlateSceneIndex::_GetAdditionalDirtyLocators() const 
{
    return HdDataSourceLocatorSet({
        HdLegacyDisplayStyleSchema::GetDefaultLocator()
    });
}
} // anonymous namespace

HdSceneIndexBaseRefPtr
HdSt_BackPlateSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);

    return HdSt_BackPlateSceneIndex::New(inputSceneIndex);
}

PXR_NAMESPACE_CLOSE_SCOPE
