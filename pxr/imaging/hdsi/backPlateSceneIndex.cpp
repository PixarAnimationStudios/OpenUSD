//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/backPlateSceneIndex.h"

#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
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
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/schemaTypeDefs.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include <vector>
PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_backPlatePrimsTokens,
    (mesh)
    (material)
);

TF_DEFINE_PRIVATE_TOKENS(_materialNodeNamesTokens,
    (UsdPrimvarReader_float2)
    (UsdUVTexture)
    (UsdUnlitSurface)
);

TF_DEFINE_PRIVATE_TOKENS(_materialNodeParamsTokens,
    (rgb)
    (a)
    (slope)
    (power)
    (offset)
    (opacity)
    (file) 
    (st)
    (wrapS)
    (wrapT)
    (clamp)
    (implementationSource)
    (backPlateColor)
    (sourceAsset)
    (varname)
    (result)
    ((glslfxSourceAsset, "glslfx:sourceAsset"))
    ((glslfxShader, "shaders/unlitShader.glslfx"))
);

//////////////////////////// Helper Functions //////////////////////////////////
static inline SdfPath
_BuildBackPlatePrimPath(const SdfPath &cameraPrimPath,
                    const TfToken &instanceName,
                    const TfToken &childName)
{
    return cameraPrimPath.AppendChild(instanceName).AppendChild(childName);
}

static inline HdBackPlateSchema 
_GetBackPlateSchema(TfToken const &instanceName, 
                    HdContainerDataSourceHandle const &primDataSource) 
{
    HdCameraSchema cam = HdCameraSchema::GetFromParent(primDataSource);
    HdBackPlateSchema backplate = cam.GetBackPlate().Get(instanceName);
    return backplate;
}

template <typename DataSourceHandle, typename T>
static bool
_GetDefaultTypedValue(const DataSourceHandle &dataSource, 
                      HdSampledDataSource::Time const shutterOffset,
                      T* result) 
{
    if (!dataSource || !result) {
        return false;
    }
    *result = dataSource->GetTypedValue(shutterOffset);
    return true;
}

static GfVec2f
_ComputeEffectiveDimensions(
    TfToken const &instanceName,
    HdContainerDataSourceHandle const &primDataSource,
    HdSampledDataSource::Time const shutterOffset) 
{
    HdCameraSchema camera = HdCameraSchema::GetFromParent(primDataSource);
    if (!camera) {
        return GfVec2f(0.0);
    }

    float horizontalAperture;
    float verticalAperture;
    TfToken projectionType;
    
    bool cameraValuesAreValid = 
        _GetDefaultTypedValue(
            camera.GetProjection(), shutterOffset, &projectionType) &&
        _GetDefaultTypedValue(
            camera.GetVerticalAperture(), shutterOffset, &verticalAperture) &&
        _GetDefaultTypedValue(
            camera.GetHorizontalAperture(), shutterOffset, &horizontalAperture);
    
    if (!cameraValuesAreValid) {
        return GfVec2f(0.0);
    }

    HdBackPlateSchema backPlate =
        _GetBackPlateSchema(instanceName, primDataSource);
    if (!backPlate) {
        return GfVec2f(0.0);
    }

    GfVec2f scale = GfVec2f(1.0);
    _GetDefaultTypedValue( backPlate.GetScaleTweak(), shutterOffset, &scale);

    // For an orthographic projection, the back plate will by default fit the 
    // camera's aperture and sized accordingly by it's scale attribute.
    if (projectionType == HdCameraSchemaTokens->orthographic) {
        return GfVec2f(horizontalAperture*scale[0],verticalAperture*scale[1]);
    }

    // For a perspective projection, the back plate will by default fit the 
    // camera's frustum and scaled further by it's scale attribute.
    float focalLength;
    float focusDistance;
    cameraValuesAreValid = 
        _GetDefaultTypedValue(
            camera.GetFocalLength(), shutterOffset, &focalLength) &&
        _GetDefaultTypedValue(
            camera.GetFocusDistance(), shutterOffset, &focusDistance);
    
    if (!cameraValuesAreValid) {
        return  GfVec2f(0.0);
    }

    // Calculate the ratio of the aperture to the focal length and then multiply
    // by the focus distance and scaling factor to get the actual dimension.
    const float ratio = focusDistance / focalLength;
    return GfVec2f(horizontalAperture*scale[0]*ratio,
                   verticalAperture*scale[1]*ratio);
}

static GfMatrix4d 
_ComputeLocalXform(HdContainerDataSourceHandle const &primDataSource,
                   TfToken const &instanceName,
                   HdSampledDataSource::Time const shutterOffset)
{
    HdCameraSchema cam = HdCameraSchema::GetFromParent(primDataSource);
    HdBackPlateSchema backPlate = 
        _GetBackPlateSchema(instanceName, primDataSource);

    if (!cam || !backPlate) {
        return GfMatrix4d(1.0);
    }

    // Translation is applied ontop of a shift to the focus distance from 
    // the camera. Refer to the docs for more details.
    float focusDistance;
    if(!_GetDefaultTypedValue(
        cam.GetFocusDistance(), shutterOffset, &focusDistance))
    {
        return GfMatrix4d(1.0);
    }
    
    GfVec3d translate(0.0);
    GfVec3f rotate(0.0);
    _GetDefaultTypedValue(
        backPlate.GetRotateXYZTweak(), shutterOffset, &rotate);
    _GetDefaultTypedValue(
        backPlate.GetTranslateTweak(), shutterOffset, &translate);
    
    // Camera is Y-up looking down the -Z axis
    translate[2] -= focusDistance;

    GfRotation rx(GfVec3d(1,0,0), rotate[0]);
    GfRotation ry(GfVec3d(0,1,0), rotate[1]);
    GfRotation rz(GfVec3d(0,0,1), rotate[2]);

    GfMatrix4d local;
    local.SetTransform(rz * ry * rx, translate);
    return local;
}
/////////////////////////// Datasource Builders ////////////////////////////////

class _PointsValueDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsValueDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }
                
    VtVec3fArray GetTypedValue(Time t) override {
        GfVec2f dims = _ComputeEffectiveDimensions( 
                            _instanceName, _primDataSource, t);
        float width = dims[0];
        float length = dims[1];
        return {
            { 0.5f * width, 0.5f * length, 0.0f},
            {-0.5f * width, 0.5f * length, 0.0f},
            {-0.5f * width,-0.5f * length, 0.0f},
            { 0.5f * width,-0.5f * length, 0.0f}};
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time> *outSampleTimes) override
    {
        HdCameraSchema cam = HdCameraSchema::GetFromParent(_primDataSource);
        HdBackPlateSchema backPlate = 
            _GetBackPlateSchema(_instanceName, _primDataSource);
        HdSampledDataSourceHandle sources[] = {
            cam.GetHorizontalAperture(),
            cam.GetVerticalAperture(),
            cam.GetFocalLength(),
            cam.GetFocusDistance(),
            cam.GetProjection(),
            backPlate.GetScaleTweak()
        };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _PointsValueDataSource(TfToken const &instanceName,
                           HdContainerDataSourceHandle const &primDataSource)
    : _instanceName(instanceName)
    , _primDataSource(primDataSource) {}

    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDataSource;
}; 

class _XformMatrixDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_XformMatrixDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    GfMatrix4d GetTypedValue(Time shutterOffset) override {
        HdXformSchema camXform = HdXformSchema::GetFromParent(_primDataSource);

        const GfMatrix4d local = 
            _ComputeLocalXform(_primDataSource, _instanceName, shutterOffset);
        GfMatrix4d camXformMatrix(1.0);

        if (camXform) {
            _GetDefaultTypedValue(camXform.GetMatrix(), shutterOffset, 
                &camXformMatrix);
        }
        return local * camXformMatrix;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdCameraSchema cam = HdCameraSchema::GetFromParent(_primDataSource);
        HdXformSchema camXform = HdXformSchema::GetFromParent(_primDataSource);
        HdBackPlateSchema backPlate = _GetBackPlateSchema(_instanceName, 
                                                          _primDataSource);
        HdSampledDataSourceHandle sources[] = {
            cam.GetFocusDistance(),
            backPlate.GetTranslateTweak(),
            backPlate.GetRotateXYZTweak(),
            camXform.GetMatrix()
        };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }           

private:
    _XformMatrixDataSource(TfToken const &instanceName,
                            HdContainerDataSourceHandle const &primDataSource)
    : _instanceName(instanceName)
    , _primDataSource(primDataSource) {}

    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDataSource;
};

class _AssetPathDataSource : public HdAssetPathDataSource 
{
public:
    HD_DECLARE_DATASOURCE(_AssetPathDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    SdfAssetPath GetTypedValue(Time shutterOffset) override {
        HdBackPlateSchema backPlate = _GetBackPlateSchema(_instanceName, 
                                                          _primDataSource);
        SdfAssetPath assetPath;
        if (!_GetDefaultTypedValue(
                backPlate.GetImage(), shutterOffset, &assetPath)){
            return SdfAssetPath();
        };
        return assetPath;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdBackPlateSchema backPlate = _GetBackPlateSchema(_instanceName, 
                                                          _primDataSource);
        HdSampledDataSourceHandle source = backPlate.GetImage();
        if (!source) {
            return false;
        }
        return source->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _AssetPathDataSource(TfToken const &instanceName,
                            HdContainerDataSourceHandle const &primDataSource)
    : _instanceName(instanceName)
    , _primDataSource(primDataSource){}

    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDataSource;
};

class _ColorGradingDataSource : public HdVec3fDataSource 
{
public:
    HD_DECLARE_DATASOURCE(_ColorGradingDataSource);

    VtValue GetValue(Time t) override {
        return VtValue(GetTypedValue(t));
    }

    GfVec3f GetTypedValue(Time shutterOffset) override {
        HdBackPlateSchema backPlate = _GetBackPlateSchema(_instanceName, 
                                                          _primDataSource);
        GfVec3f result(0.0);
        // if _type is offset we leave the default as (0, 0, 0).
        if (_type == _materialNodeParamsTokens->slope || 
            _type == _materialNodeParamsTokens->power) {
            result = GfVec3f(1.0);
        }

        if (!_GetDefaultTypedValue(
                _GetLumaSource(backPlate), shutterOffset, &result)) {
            return result;
        }
        return result;
    }

    bool GetContributingSampleTimesForInterval(
            Time startTime, Time endTime,
            std::vector<Time> *outSampleTimes) override
    {
        HdBackPlateSchema backPlate = _GetBackPlateSchema(_instanceName, 
                                                          _primDataSource);
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
                            HdContainerDataSourceHandle const &primDataSource)
    : _instanceName(instanceName)
    , _type(type)
    , _primDataSource(primDataSource){}

    HdVec3fDataSourceHandle 
    _GetLumaSource(HdBackPlateSchema const &backPlate) const {
        if (_type == _materialNodeParamsTokens->offset) {
            return backPlate.GetLumaOffset();
        }
        if (_type == _materialNodeParamsTokens->slope) {
            return backPlate.GetLumaSlope();
        }
        if (_type == _materialNodeParamsTokens->power) {
            return backPlate.GetLumaPower();
        }
        return nullptr;
    }

    TfToken const _instanceName;
    TfToken const _type;
    HdContainerDataSourceHandle const _primDataSource;
};

static HdContainerDataSourceHandle
_BuildMeshTopologyDataSource()
{
    static const VtIntArray numVerts{4};
    static const VtIntArray verts{0, 1, 2, 3};

    static const HdContainerDataSourceHandle topologyDs =
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

static HdContainerDataSourceHandle
_BuildMaterialNetworkDataSource(
    TfToken const &appliedInstanceName,
    HdContainerDataSourceHandle const &primDataSource)
{
    // USD Primar Reader Params
    static const HdDataSourceBaseHandle usdPrimvarReaderParams = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<VtValue>::New(
                VtValue(_materialNodeParamsTokens->st)))
            .Build()};

    // USD Primar Reader Node
    const HdDataSourceBaseHandle usdPrimvarReaderNode
        = HdMaterialNodeSchema::Builder()
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    _materialNodeParamsTokens->varname,
                    usdPrimvarReaderParams))
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    _materialNodeNamesTokens->UsdPrimvarReader_float2))
            .Build();

    /// Usd UVTextureNode Params
    static const TfToken texParamNames[] = { 
        _materialNodeParamsTokens->file, 
        _materialNodeParamsTokens->st, 
        _materialNodeParamsTokens->wrapS, 
        _materialNodeParamsTokens->wrapT };
    _AssetPathDataSource::Handle assetPathDs =
         _AssetPathDataSource::New(appliedInstanceName, primDataSource);
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

    // Usd UVTextureNode Connections
    static const TfToken texConnectionNames[] = {_materialNodeParamsTokens->st};
    HdDataSourceBaseHandle texConnections[] = {
        HdRetainedSmallVectorDataSource::New(
            1,
            std::array<HdDataSourceBaseHandle, 1>{
                HdMaterialConnectionSchema::Builder()
                    .SetUpstreamNodePath(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeNamesTokens->UsdPrimvarReader_float2))
                    .SetUpstreamNodeOutputName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeParamsTokens->result))
                    .Build()
            }.data())
    };
    // UsdUVTextureNode 
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

    // USD Unlit Surface Connections
    static const TfToken usdUnlitSurfaceConnectionNames[] = {
        _materialNodeParamsTokens->backPlateColor,
        _materialNodeParamsTokens->opacity
    };
    static const HdDataSourceBaseHandle surfaceConnection = {
        HdRetainedSmallVectorDataSource::New(
            1,
            std::array<HdDataSourceBaseHandle, 1> {
                HdMaterialConnectionSchema::Builder()
                    .SetUpstreamNodePath(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeNamesTokens->UsdUVTexture))
                    .SetUpstreamNodeOutputName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeParamsTokens->rgb))
                    .Build()}.data())};
    static const HdDataSourceBaseHandle opacityConnection = {
        HdRetainedSmallVectorDataSource::New(
            1,
            std::array<HdDataSourceBaseHandle, 1> {
                HdMaterialConnectionSchema::Builder()
                    .SetUpstreamNodePath(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeNamesTokens->UsdUVTexture))
                    .SetUpstreamNodeOutputName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _materialNodeParamsTokens->a))
                    .Build() }
                .data())
    };
    static const HdDataSourceBaseHandle usdUnlitSurfaceConnections[] = {
        surfaceConnection,
        opacityConnection
    };

    static const TfToken usdUnlitSurfaceParamNames[] = {
        _materialNodeParamsTokens->offset,
        _materialNodeParamsTokens->slope,
        _materialNodeParamsTokens->power,
        _materialNodeParamsTokens->opacity
    };
    const HdDataSourceBaseHandle usdUnlitSurfaceParams[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _materialNodeParamsTokens->offset, 
                primDataSource))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _materialNodeParamsTokens->slope, 
                primDataSource))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(_ColorGradingDataSource::New(
                appliedInstanceName, 
                _materialNodeParamsTokens->power, 
                primDataSource))
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
    const int paramNum = assetHasOpacity ? 3 : 4;
    const int connNum = assetHasOpacity ? 2 : 1;

    const HdDataSourceBaseHandle usdUnlitSurfaceNode =
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
    const HdContainerDataSourceHandle nodesDs
        = HdRetainedContainerDataSource::New(
            _materialNodeNamesTokens->UsdPrimvarReader_float2, 
                usdPrimvarReaderNode,
            _materialNodeNamesTokens->UsdUnlitSurface, usdUnlitSurfaceNode,
            _materialNodeNamesTokens->UsdUVTexture, textureNode);

    // USD Unlit Surface Terminals
    static const HdContainerDataSourceHandle terminalsDs
        = HdRetainedContainerDataSource::New(
            HdMaterialTerminalTokens->surface,
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _materialNodeNamesTokens->UsdUnlitSurface))
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
                                                   _primDataSource);
        }
        return nullptr;
    }

private:
    _MaterialDataSource(
        TfToken const & instanceName,
        HdContainerDataSourceHandle const &primDataSource)
     : _instanceName(instanceName),
       _primDataSource(primDataSource)
    {}
    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDataSource;
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
                    _PointsValueDataSource::New(_instanceName, _primDataSource))
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
            static const VtVec2fArray textureCoord {
                {1.0, 1.0},
                {0.0, 1.0},
                {0.0, 0.0},
                {1.0,0.0}};

            static const HdContainerDataSourceHandle stPrimvarDs =
                HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec2fArray>::New(
                        textureCoord))
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
        HdContainerDataSourceHandle const &primDataSource)
     : _instanceName(instanceName),
       _primDataSource(primDataSource)
    {
    }
    TfToken const _instanceName;
    HdContainerDataSourceHandle const _primDataSource;
};

static HdDataSourceBaseHandle
_BuildXformDataSource(
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDataSource) {

    HdDataSourceBaseHandle xformDs = HdXformSchema::Builder()
        .SetMatrix(
            _XformMatrixDataSource::New(instanceName, primDataSource))
        .SetResetXformStack(
            HdRetainedTypedSampledDataSource<bool>::New(false))
        .Build();
    return xformDs;
}

static HdContainerDataSourceHandle
_BuildMaterialBindingsDataSource(const SdfPath &platePath)
{
    SdfPath materialPath =
        platePath.AppendChild(_backPlatePrimsTokens->material);
    HdDataSourceBaseHandle materialBindingDs = 
                HdMaterialBindingSchema::Builder()
                    .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(
                        materialPath))
                    .Build();
    return HdRetainedContainerDataSource::New(
            HdMaterialBindingsSchemaTokens->allPurpose,
            materialBindingDs);
}

static HdContainerDataSourceHandle
_BuildMeshDataSource()
{
    return HdRetainedContainerDataSource::New( 
        HdMeshSchemaTokens->topology,
            _BuildMeshTopologyDataSource(),
        HdMeshSchemaTokens->subdivisionScheme,
            HdRetainedTypedSampledDataSource<TfToken>::New(
                PxOsdOpenSubdivTokens->catmullClark),
        HdMeshSchemaTokens->doubleSided,
            HdRetainedTypedSampledDataSource<bool>::New(false));
}

static HdContainerDataSourceHandle
_BuildMeshPrimDataSource(
    const SdfPath &platePath,
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdMeshSchemaTokens->mesh,
                _BuildMeshDataSource(),
            HdPrimvarsSchemaTokens->primvars,
                _PrimvarsDataSource::New(instanceName, primDataSource),
            HdXformSchemaTokens->xform,
                _BuildXformDataSource(instanceName, primDataSource),
            HdMaterialBindingsSchema::GetSchemaToken(),
                _BuildMaterialBindingsDataSource(platePath));
}

static HdContainerDataSourceHandle
_BuildMaterialDataSource(
    const TfToken &instanceName,
    const HdContainerDataSourceHandle &primDataSource)
{
    return _MaterialDataSource::New(instanceName, primDataSource);
}

////////////////////////////////////////////////////////////////////////////////

/* static */
HdsiBackPlateSceneIndexRefPtr
HdsiBackPlateSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiBackPlateSceneIndex(inputSceneIndex));
}

HdsiBackPlateSceneIndex::HdsiBackPlateSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

static HdSceneIndexObserver::AddedPrimEntries
_ToAddedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::AddedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        if (path.GetElementToken() == _backPlatePrimsTokens->mesh) {
            entries.push_back({path, HdPrimTypeTokens->mesh});
            continue;
        }
        if (path.GetElementToken() == _backPlatePrimsTokens->material) {
            entries.push_back({path, HdPrimTypeTokens->material});
            continue;
        }
    }
    return entries;
}

static HdSceneIndexObserver::RemovedPrimEntries
_ToRemovedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::RemovedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        entries.push_back({path});
    }
    return entries;
}

HdSceneIndexPrim
HdsiBackPlateSceneIndex::_GetMeshOrMaterialSiPrim(
    const SdfPath &plateInstancePath,
    const TfToken &plateChildType) const
{
    const TfToken instanceName = plateInstancePath.GetElementToken();

    const SdfPath cameraPrimPath = plateInstancePath.GetParentPath();
    HdContainerDataSourceHandle cameraDs = 
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource;
    if (plateChildType == _backPlatePrimsTokens->mesh) {
        return {HdPrimTypeTokens->mesh, _BuildMeshPrimDataSource(
            plateInstancePath, instanceName, cameraDs)};
    }
    if (plateChildType == _backPlatePrimsTokens->material) {
        return {HdPrimTypeTokens->material,
            _BuildMaterialDataSource(instanceName, cameraDs)};
    }
    return HdSceneIndexPrim();
}

bool
HdsiBackPlateSceneIndex::_IsBackPlateInMap(const SdfPath &platePath) const {
    const SdfPath cameraPath = platePath.GetParentPath();
    const auto it = _cameraToBackPlates.find(cameraPath);
    if (it != _cameraToBackPlates.end() && !it->second.empty()) {
        const _BackPlates &backPlates = it->second;
        const TfToken instanceName = platePath.GetElementToken();
        const auto plateIt = backPlates.find(instanceName);
        return plateIt != backPlates.end();
    }
    return false;
}

HdSceneIndexPrim
HdsiBackPlateSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();
    if (primPath.IsAbsoluteRootPath()) {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    // Check if we have back plate prim path and return an empty prim if so.
    if (_IsBackPlateInMap(primPath)) {
        return HdSceneIndexPrim({TfToken(),
            HdRetainedContainerDataSource::New()});
    }
    
    // Check if we have a back plate child prim path
    const SdfPath platePath = primPath.GetParentPath();
    if (_IsBackPlateInMap(platePath)) {
        HdSceneIndexPrim prim = 
            _GetMeshOrMaterialSiPrim(platePath, primPath.GetElementToken());
        if (prim) {
            return prim;
        }
    }
    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdsiBackPlateSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Case 1: The prim path is the camera prim's path. Return all back plate
    // instances.
    const auto it = _cameraToBackPlates.find(primPath);
    if (it != _cameraToBackPlates.end()) {
        SdfPathVector paths =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        for (const TfToken &name : it->second) {
            paths.push_back(primPath.AppendChild(name));
        }
        return paths;
    }

    // Case 2: The prim path is a back plate path. Return mesh and material
    // children prims.
    if (_IsBackPlateInMap(primPath)){
        const SdfPath meshPath = 
            primPath.AppendChild(_backPlatePrimsTokens->mesh);
        const SdfPath materialPath = 
            primPath.AppendChild(_backPlatePrimsTokens->material);
        return {meshPath, materialPath};
    }

    // Case 3: Any other prim including back plate mesh/material prims
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiBackPlateSceneIndex::_AddBackPlateChildren(
    const SdfPath &cameraPrimPath,
    SdfPathSet * const addedBackPlatePrims)
{
    HdCameraSchema cameraSchema = HdCameraSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource);
    if (!cameraSchema) {
        return;
    }

    HdBackPlateContainerSchema backPlatesSchema = cameraSchema.GetBackPlate();
    TfTokenVector names = backPlatesSchema.GetNames();
    
    _BackPlates backPlates;
    for (const TfToken & name : names){
        if (addedBackPlatePrims) {
            const SdfPath meshPath = _BuildBackPlatePrimPath(cameraPrimPath,
                name,_backPlatePrimsTokens->mesh);
            const SdfPath materialPath = _BuildBackPlatePrimPath(cameraPrimPath,
                name, _backPlatePrimsTokens->material);
            addedBackPlatePrims->insert(cameraPrimPath.AppendChild(name));
            addedBackPlatePrims->insert(meshPath);
            addedBackPlatePrims->insert(materialPath);
        }
        backPlates.insert(name);
    }
    if (!backPlates.empty()) {
        _cameraToBackPlates.emplace(cameraPrimPath, backPlates);
    }
}

void
HdsiBackPlateSceneIndex::_DirtyBackPlateChildren(
    const SdfPath &cameraPrimPath,
    HdSceneIndexObserver::DirtiedPrimEntries * const dirtiedBackPlatePrims)
{
    if (!dirtiedBackPlatePrims) {
        return;
    }

    HdCameraSchema cameraSchema = HdCameraSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(cameraPrimPath).dataSource);
    if (!cameraSchema) {
        return;
    }

    HdBackPlateContainerSchema backPlatesSchema = cameraSchema.GetBackPlate();
    TfTokenVector names = backPlatesSchema.GetNames();
    
    for (const TfToken & name : names){
        const SdfPath meshPath = _BuildBackPlatePrimPath(cameraPrimPath, name,
            _backPlatePrimsTokens->mesh);
        const SdfPath materialPath = _BuildBackPlatePrimPath(
            cameraPrimPath, name, _backPlatePrimsTokens->material);
        dirtiedBackPlatePrims->push_back({
            meshPath,
            HdDataSourceLocatorSet({
                HdMaterialSchema::GetDefaultLocator(),
                HdPrimvarsSchema::GetDefaultLocator(),
                HdXformSchema::GetDefaultLocator()})
        });
        dirtiedBackPlatePrims->push_back({
            materialPath,
            HdDataSourceLocatorSet({
                HdMaterialSchema::GetDefaultLocator()})
        });
    }
}

void
HdsiBackPlateSceneIndex::_RemoveBackPlateChildren(
    const SdfPath &primPath,
    SdfPathSet * const removedBackPlatePrims)
{
    auto it = _cameraToBackPlates.lower_bound(primPath);
    while (it != _cameraToBackPlates.end() && 
            it->first.HasPrefix(primPath)) {
        if (removedBackPlatePrims) {
            for (const TfToken & name : it->second) {
                removedBackPlatePrims->insert(
                    it->first.AppendChild(name));
            }
        }
        it = _cameraToBackPlates.erase(it);
    }
}

void
HdsiBackPlateSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    SdfPathSet addedBackPlatePrims;
    SdfPathSet removedBackPlatePrims;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        _RemoveBackPlateChildren(
            entry.primPath,
            isObserved ? &removedBackPlatePrims : nullptr);
        _AddBackPlateChildren(
            entry.primPath,
            isObserved ? &addedBackPlatePrims : nullptr);
    }
    if (!isObserved) {
        return;
    }

    _SendPrimsAdded(entries);

    if (!removedBackPlatePrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedBackPlatePrims));
    }

    if (!addedBackPlatePrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedBackPlatePrims));
    }

}

void
HdsiBackPlateSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_cameraToBackPlates.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            _RemoveBackPlateChildren(entry.primPath, nullptr);
        }
    }

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiBackPlateSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedBackPlatePrims;
    SdfPathSet addedBackPlatePrims;
    SdfPathSet removedBackPlatePrims;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Intersects(
                HdCameraSchema::GetBackPlateLocator())) {
            _RemoveBackPlateChildren(
                entry.primPath,
                isObserved ? &removedBackPlatePrims : nullptr);
            _AddBackPlateChildren(
                entry.primPath,
                isObserved ? &addedBackPlatePrims : nullptr);
        }
        if ((!_cameraToBackPlates.empty()) && 
            (entry.dirtyLocators.Intersects(
                HdCameraSchema::GetDefaultLocator()) ||
            entry.dirtyLocators.Intersects(
                HdXformSchema::GetDefaultLocator()))) {
            _DirtyBackPlateChildren(
                entry.primPath,
                isObserved ? &dirtiedBackPlatePrims : nullptr);
        }
    }

    if (!isObserved) {
        return;
    }

    _SendPrimsDirtied(entries);

    if (!dirtiedBackPlatePrims.empty()) {
        _SendPrimsDirtied(dirtiedBackPlatePrims);
    }
    if (!removedBackPlatePrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedBackPlatePrims));
    }
    if (!addedBackPlatePrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedBackPlatePrims));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

