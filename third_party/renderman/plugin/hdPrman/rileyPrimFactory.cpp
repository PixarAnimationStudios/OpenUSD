//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/rileyPrimFactory.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyCameraPrim.h"
#include "hdPrman/rileyClippingPlanePrim.h"
#include "hdPrman/rileyCoordinateSystemPrim.h"
#include "hdPrman/rileyDisplacementPrim.h"
#include "hdPrman/rileyDisplayPrim.h"
#include "hdPrman/rileyDisplayFilterPrim.h"
#include "hdPrman/rileyGeometryInstancePrim.h"
#include "hdPrman/rileyGeometryPrototypePrim.h"
#include "hdPrman/rileyGlobalsPrim.h"
#include "hdPrman/rileyIntegratorPrim.h"
#include "hdPrman/rileyLightInstancePrim.h"
#include "hdPrman/rileyLightShaderPrim.h"
#include "hdPrman/rileyMaterialPrim.h"
#include "hdPrman/rileyRenderOutputPrim.h"
#include "hdPrman/rileyRenderTargetPrim.h"
#include "hdPrman/rileyRenderViewPrim.h"
#include "hdPrman/rileySampleFilterPrim.h"

#include "hdPrman/tokens.h"

#include "pxr/imaging/hdsi/primTypeNoticeBatchingSceneIndex.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

HdContainerDataSourceHandle
_GetPrimSource(
    const HdsiPrimManagingSceneIndexObserver * const observer,
    const SdfPath &path)
{
    return observer->GetSceneIndex()->GetPrim(path).dataSource;
}

class _RileyPrimTypePriorityFunctor
    : public HdsiPrimTypeNoticeBatchingSceneIndex::PrimTypePriorityFunctor
{
    size_t GetPriorityForPrimType(
        const TfToken &primType) const override
    {
        /// Dependencies are as follows:
        ///
        /// Riley::SetOptions needs to be before anything else!
        ///
        /// lightShader     <----------------------------< lightInstance
        ///                                               /
        /// material      <------------------------------*---< geometryInstance
        ///                                             /
        /// coordinateSystem  <------------------------*
        ///                                           /
        /// displacement  <----< geometryPrototype <-*
        ///
        /// clippingPlane
        ///
        /// renderOutput <-------------------------------< display
        ///             \                                 / 
        ///              *-----<  renderTarget <---------*
        ///                                   \                            _
        /// integrator <-----------------------*
        ///                                     \                          _
        /// displayFilter <----------------------*---------< renderView
        ///                                     /
        /// sampleFilter <---------------------*
        ///                                   /
        /// camera  <------------------------*
        if (primType == HdPrmanRileyPrimTypeTokens->globals) {
            return 0;
        }

        if (primType == HdPrmanRileyPrimTypeTokens->lightShader ||
            primType == HdPrmanRileyPrimTypeTokens->material ||
            primType == HdPrmanRileyPrimTypeTokens->coordinateSystem ||
            primType == HdPrmanRileyPrimTypeTokens->displacement ||
            primType == HdPrmanRileyPrimTypeTokens->clippingPlane ||
            primType == HdPrmanRileyPrimTypeTokens->renderOutput ||
            primType == HdPrmanRileyPrimTypeTokens->integrator||
            primType == HdPrmanRileyPrimTypeTokens->displayFilter||
            primType == HdPrmanRileyPrimTypeTokens->sampleFilter||
            primType == HdPrmanRileyPrimTypeTokens->camera) {
            return 1;
        }

        if (primType == HdPrmanRileyPrimTypeTokens->geometryPrototype ||
            primType == HdPrmanRileyPrimTypeTokens->renderTarget) {
            return 2;
        }

        if (primType == HdPrmanRileyPrimTypeTokens->lightInstance ||
            primType == HdPrmanRileyPrimTypeTokens->geometryInstance ||
            primType == HdPrmanRileyPrimTypeTokens->display ||
            primType == HdPrmanRileyPrimTypeTokens->renderView) {
            return 3;
        }

        return 4;
    }

    size_t GetNumPriorities() const override
    {
        return 5;
    }
};

}

HdPrman_RileyPrimFactory::HdPrman_RileyPrimFactory(
    HdPrman_RenderParam * const renderParam)
  : _renderParam(renderParam)
{
}

HdsiPrimManagingSceneIndexObserver::PrimBaseHandle
HdPrman_RileyPrimFactory::CreatePrim(
    const HdSceneIndexObserver::AddedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    if (entry.primType == HdPrmanRileyPrimTypeTokens->camera) {
        return std::make_shared<HdPrman_RileyCameraPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->clippingPlane) {
        return std::make_shared<HdPrman_RileyClippingPlanePrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->coordinateSystem) {
        return std::make_shared<HdPrman_RileyCoordinateSystemPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->displacement) {
        return std::make_shared<HdPrman_RileyDisplacementPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->display) {
        return std::make_shared<HdPrman_RileyDisplayPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->displayFilter) {
        return std::make_shared<HdPrman_RileyDisplayFilterPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->geometryInstance) {
        return std::make_shared<HdPrman_RileyGeometryInstancePrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->geometryPrototype) {
        return std::make_shared<HdPrman_RileyGeometryPrototypePrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->globals) {
        return std::make_shared<HdPrman_RileyGlobalsPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->integrator) {
        return std::make_shared<HdPrman_RileyIntegratorPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->lightInstance) {
        return std::make_shared<HdPrman_RileyLightInstancePrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->lightShader) {
        return std::make_shared<HdPrman_RileyLightShaderPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->material) {
        return std::make_shared<HdPrman_RileyMaterialPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->renderOutput) {
        return std::make_shared<HdPrman_RileyRenderOutputPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->renderTarget) {
        return std::make_shared<HdPrman_RileyRenderTargetPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->renderView) {
        return std::make_shared<HdPrman_RileyRenderViewPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->sampleFilter) {
        return std::make_shared<HdPrman_RileySampleFilterPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    return nullptr;
}

const HdContainerDataSourceHandle &
HdPrman_RileyPrimFactory::GetPrimTypeNoticeBatchingSceneIndexInputArgs()
{
    using PriorityFunctorHandle =
        HdsiPrimTypeNoticeBatchingSceneIndex::PrimTypePriorityFunctorHandle;

    static const HdContainerDataSourceHandle result =
        HdRetainedContainerDataSource::New(
            HdsiPrimTypeNoticeBatchingSceneIndexTokens
                                ->primTypePriorityFunctor,
            HdRetainedTypedSampledDataSource<PriorityFunctorHandle>::New(
                std::make_shared<_RileyPrimTypePriorityFunctor>()));
    
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
