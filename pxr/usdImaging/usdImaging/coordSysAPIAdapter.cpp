//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/coordSysAPIAdapter.h"

#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (coordSysBinding_dep_xform)
);

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCoordSysAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

// DataSource to hold coordSysAPI
class _CoordsSysContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordsSysContainerDataSource);

    _CoordsSysContainerDataSource(
            const UsdPrim &prim,
            const TfToken &name,
            const UsdImagingDataSourceStageGlobals &stageGlobals) :
        _coordSysAPI(prim, name),
        _stageGlobals(stageGlobals) {}

    TfTokenVector GetNames() override {
        return { _coordSysAPI.GetName() };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _coordSysAPI.GetName()) {
            UsdShadeCoordSysAPI::Binding binding = 
                _coordSysAPI.GetLocalBinding();
            if (!binding.name.IsEmpty()) {
                return HdRetainedTypedSampledDataSource<SdfPath>::New(
                        binding.bindingRelPath);
            }
        }
        return nullptr;
    }

private:
    UsdShadeCoordSysAPI _coordSysAPI;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_CoordsSysContainerDataSource);


// DataSource to hold information of the coordSys subprim
// - Following information is extracted from the applied UsdShadeCoordSysAPI
//  - name: instance name of the multi-applied CoordSysAPI schema.
//  - xform: from the target bound on the coordSysAPI binding relationship.
class UsdImagingDataSourceCoordsysPrim : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceCoordsysPrim);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            HdCoordSysSchemaTokens->coordSys,
            HdXformSchemaTokens->xform,
            HdDependenciesSchemaTokens->__dependencies
        };
        return names;
    }
    
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdCoordSysSchemaTokens->coordSys) {
            return HdCoordSysSchema::Builder()
                .SetName(HdRetainedTypedSampledDataSource<TfToken>::New(
                        _coordSysName))
                .Build();
        }

        if (name == HdXformSchemaTokens->xform) {
            UsdGeomXformable xformable(_boundXformPrim);
            if (!xformable) {
                return nullptr;
            }
            UsdGeomXformable::XformQuery xformQuery(xformable);
            if (!xformQuery.HasNonEmptyXformOpOrder()) {
                return nullptr;
            }
            return UsdImagingDataSourceXform::New(
                    xformQuery, _sceneIndexPath, _stageGlobals);
        }

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            // Need to add dependency for coordSys type prim (our subprim here)
            // - xform 
            // Note that there is no dependency on the coordSys binding's
            // name, as any update to this in the source usd will result in
            // invalidation of other parts of the hydra pipeline, which will
            // anyhow result in coord sys prim being dirtied.
            //
            static const HdLocatorDataSourceHandle xformDs =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdXformSchema::GetDefaultLocator());
            
            static const TfToken dependNames[] = {
                _tokens->coordSysBinding_dep_xform
            };

            const HdDataSourceBaseHandle dependValues[] = {
                //xform
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                            HdRetainedTypedSampledDataSource<SdfPath>::New(
                                _boundXformPrim.GetPath()))
                    .SetDependedOnDataSourceLocator(xformDs)
                    .SetAffectedDataSourceLocator(xformDs)
                    .Build()
            };

            HdRetainedContainerDataSourceHandle deps =
                HdRetainedContainerDataSource::New(
                        TfArraySize(dependNames), dependNames, dependValues);

            return deps;
        }

        return nullptr;
    }

private:
    UsdImagingDataSourceCoordsysPrim(
            const TfToken &appliedInstanceName,
            const SdfPath &sceneIndexPath,
            UsdPrim boundXformPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) :
        _coordSysName(appliedInstanceName),
        _sceneIndexPath(sceneIndexPath),
        _boundXformPrim(boundXformPrim),
        _stageGlobals(stageGlobals)
    {}

    const TfToken _coordSysName;
    const SdfPath _sceneIndexPath;
    UsdPrim _boundXformPrim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceCoordsysPrim);

} // anonymous namespace

// ----------------------------------------------------------------------------

std::string
_GetCoordSysName(const TfToken &appliedInstanceName)
{
    const TfTokenVector &tokens =
        { HdPrimTypeTokens->coordSys, appliedInstanceName };
    return SdfPath::JoinIdentifier(tokens);
}


TfTokenVector
UsdImagingCoordSysAPIAdapter::GetImagingSubprims(
        UsdPrim const &prim, 
        TfToken const &appliedInstanceName) 
{
    return { TfToken(_GetCoordSysName(appliedInstanceName)) };
}

TfToken
UsdImagingCoordSysAPIAdapter::GetImagingSubprimType(
        UsdPrim const &prim,
        TfToken const &subprim,
        TfToken const &appliedInstanceName)
{
    if (subprim == _GetCoordSysName(appliedInstanceName)) {
        return HdPrimTypeTokens->coordSys;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCoordSysAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    const std::string& coordSysName = _GetCoordSysName(appliedInstanceName);

    if (subprim == coordSysName) {
        // If its the binding subprim
        // - get local binding
        // - get boundXformPrim from local binding
        // - create UsdImagingDataSourceCoordsysPrim using the
        // appliedInstanceName and boundXformPrim.
        UsdShadeCoordSysAPI::Binding binding =
            UsdShadeCoordSysAPI::Apply(
                    prim, appliedInstanceName).GetLocalBinding();
        if (binding.name.IsEmpty()) {
            return nullptr;
        }

        const UsdPrim &boundXformPrim = prim.GetStage()->GetPrimAtPath(
                binding.coordSysPrimPath);

        return UsdImagingDataSourceCoordsysPrim::New(
                    appliedInstanceName,
                    prim.GetPath().AppendProperty(TfToken(coordSysName)), 
                    boundXformPrim, 
                    stageGlobals);
    }

    if (subprim.IsEmpty()) {
        return HdRetainedContainerDataSource::New(
            HdCoordSysBindingSchemaTokens->coordSysBinding,
            _CoordsSysContainerDataSource::New(
                prim, appliedInstanceName, stageGlobals)
        );
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCoordSysAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties)
{
    if (appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    const std::string& coordSysName = _GetCoordSysName(appliedInstanceName);

    if (subprim == coordSysName) {
        for (const TfToken &propertyName : properties) {
            if (propertyName == coordSysName) {
                // coordSys prim's properties include only the name and xform
                // Just invalidate on the entire hydra coordSys prim.
                return { HdDataSourceLocator::EmptyLocator() };
            }
        }
    }

    if (subprim.IsEmpty()) {
        for (const TfToken &propertyName : properties) {
            if (propertyName == coordSysName) {
                return 
                    HdCoordSysBindingSchema::GetDefaultLocator().Append(
                            appliedInstanceName);
            }
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
