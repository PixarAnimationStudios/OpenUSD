//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include <iostream>
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/renderDelegate.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/drawTarget.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/plugin/LoFi/mesh.h"
#include "pxr/imaging/plugin/LoFi/points.h"
#include "pxr/imaging/plugin/LoFi/curves.h"
#include "pxr/imaging/plugin/LoFi/instancer.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(LoFiRenderSettingsTokens, LOFI_RENDER_SETTINGS_TOKENS);


const TfTokenVector LoFiRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
  HdPrimTypeTokens->mesh,
  HdPrimTypeTokens->points,
  HdPrimTypeTokens->basisCurves
};

const TfTokenVector LoFiRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
  HdPrimTypeTokens->camera,
  HdPrimTypeTokens->drawTarget
};

const TfTokenVector LoFiRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

std::mutex LoFiRenderDelegate::_mutexResourceRegistry;
std::atomic_int LoFiRenderDelegate::_counterResourceRegistry;
LoFiResourceRegistrySharedPtr LoFiRenderDelegate::_resourceRegistry;

using LoFiResourceRegistryWeakPtr =  std::weak_ptr<LoFiResourceRegistry>;

namespace {

//
// Map from Hgi instances to resource registries.
//
// An entry is kept alive until the last shared_ptr to a resource
// registry is dropped.
//
class _HgiToResourceRegistryMap final
{
public:
    // Map is a singleton.
    static _HgiToResourceRegistryMap &GetInstance()
    {
        static _HgiToResourceRegistryMap instance;
        return instance;
    }

    // Look-up resource registry by Hgi instance, create resource
    // registry for the instance if it didn't exist.
    LoFiResourceRegistrySharedPtr GetOrCreateRegistry(Hgi * const hgi)
    {
        std::lock_guard<std::mutex> guard(_mutex);

        // Previous entry exists, use it.
        auto it = _map.find(hgi);
        if (it != _map.end()) {
            LoFiResourceRegistryWeakPtr const &registry = it->second;
            return LoFiResourceRegistrySharedPtr(registry);
        }

        // Create resource registry, custom deleter to remove corresponding
        // entry from map.
        LoFiResourceRegistrySharedPtr const result(
            new LoFiResourceRegistry(hgi),
            [this](LoFiResourceRegistry *registry) {
                this->_Destroy(registry); });

        // Insert into map.
        _map.insert({hgi, result});

        // Also register with HdPerfLog.
        //
        HdPerfLog::GetInstance().AddResourceRegistry(result.get());

        return result;
    }

private:
    void _Destroy(LoFiResourceRegistry * const registry)
    {
        TRACE_FUNCTION();

        std::lock_guard<std::mutex> guard(_mutex);

        HdPerfLog::GetInstance().RemoveResourceRegistry(registry);
        
        _map.erase(registry->GetHgi());
        delete registry;
    }

    using _Map = std::unordered_map<Hgi*, LoFiResourceRegistryWeakPtr>;

    _HgiToResourceRegistryMap() = default;

    std::mutex _mutex;
    _Map _map;
};

}

LoFiRenderDelegate::LoFiRenderDelegate()
  : HdRenderDelegate()
  , _hgi(nullptr)
{
  _Initialize();
}

LoFiRenderDelegate::LoFiRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
    , _hgi(nullptr)
{
  _Initialize();
}

void
LoFiRenderDelegate::_Initialize()
{
  // Initialize one resource registry for all St plugins
  // It will also add the resource to the logging object so we
  // can query the resources used by all St plugins later
  std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
  
  if (_counterResourceRegistry.fetch_add(1) == 0) {
    _resourceRegistry.reset( new LoFiResourceRegistry() );
    HdPerfLog::GetInstance().AddResourceRegistry(_resourceRegistry.get());
  } 

  // Create the RenderPassState object
  _renderPassState = CreateRenderPassState();
  std::cout << "LOFI DELEGATE INITIALIZED !" << std::endl;

}

LoFiRenderDelegate::~LoFiRenderDelegate()
{
  // Clean the resource registry only when it is the last LoFi delegate
  {
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    if (_counterResourceRegistry.fetch_sub(1) == 1) {
      _resourceRegistry->GarbageCollect();
      _resourceRegistry.reset();
    }
  }
}

void
LoFiRenderDelegate::SetDrivers(HdDriverVector const& drivers)
{
    // For LoFi we want to use the Hgi driver, so extract it.
    for (HdDriver* hdDriver : drivers) {
        if (hdDriver->name == HgiTokens->renderDriver &&
            hdDriver->driver.IsHolding<Hgi*>()) {
            _hgi = hdDriver->driver.UncheckedGet<Hgi*>();
            break;
        }
    }
    TF_VERIFY(_hgi, "LoFi requires Hgi HdDriver");
}

void 
LoFiRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
  std::cout << "LOFI DELEGATE RESOURCE COMMIT STARTED !" << std::endl;
  _resourceRegistry->Commit();

  //if (tracker->IsGarbageCollectionNeeded()) {
  _resourceRegistry->GarbageCollect();
  std::cout << "LOFI DELEGATE RESOURCE COMMITED !" << std::endl;
  //  tracker->ClearGarbageCollectionNeeded();
  //}  
}

HdRenderSettingDescriptorList
LoFiRenderDelegate::GetRenderSettingDescriptors() const
{
  return _settingDescriptors;
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedRprimTypes() const
{
  return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedSprimTypes() const
{
  return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedBprimTypes() const
{
  return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
LoFiRenderDelegate::GetResourceRegistry() const
{
  return _resourceRegistry;
}

HdRenderPassSharedPtr 
LoFiRenderDelegate::CreateRenderPass(
  HdRenderIndex *index,
  HdRprimCollection const& collection)
{
  return HdRenderPassSharedPtr(new LoFiRenderPass(index, collection));  
}

HdRprim *
LoFiRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
  std::cout << "LOFI DELEGATE CREATE RPRIM !" << std::endl;
  if (typeId == HdPrimTypeTokens->mesh) {
    return new LoFiMesh(rprimId);
  } else if(typeId == HdPrimTypeTokens->points) {
    return new LoFiPoints(rprimId);
  } else if(typeId == HdPrimTypeTokens->basisCurves) {
    return new LoFiCurves(rprimId);
  } else {
    TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
      typeId.GetText(), 
      rprimId.GetText());
  }
  return nullptr;
}

void
LoFiRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
  delete rPrim;
}

HdSprim *
LoFiRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
  if (typeId == HdPrimTypeTokens->camera) {
    return new HdCamera(sprimId);
  } else  if (typeId == HdPrimTypeTokens->drawTarget) {
    return new LoFiDrawTarget(sprimId);
  } else {
    TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
  }

  return nullptr;
}

HdSprim *
LoFiRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
  // For fallback sprims, create objects with an empty scene path.
  // They'll use default values and won't be updated by a scene delegate.
  if (typeId == HdPrimTypeTokens->camera) {
    return new HdCamera(SdfPath::EmptyPath());
  } else  if (typeId == HdPrimTypeTokens->drawTarget) {
    return new LoFiDrawTarget(SdfPath::EmptyPath());
  }
  else {
    TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
  }

  return nullptr;
}

void
LoFiRenderDelegate::DestroySprim(HdSprim *sPrim)
{
  delete sPrim;
}

HdBprim *
LoFiRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
  TF_CODING_ERROR("Unknown Bprim type=%s id=%s", 
    typeId.GetText(), 
    bprimId.GetText());
  return nullptr;
}

HdBprim *
LoFiRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
  TF_CODING_ERROR("Creating unknown fallback bprim type=%s", 
    typeId.GetText()); 
  return nullptr;
}

void
LoFiRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
  TF_CODING_ERROR("Destroy Bprim not supported");
}

HdInstancer *
LoFiRenderDelegate::CreateInstancer(
  HdSceneDelegate *delegate,
  SdfPath const& id)
{
  return new LoFiInstancer(delegate, id);
}

void 
LoFiRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
  delete instancer;
}

Hgi*
LoFiRenderDelegate::GetHgi()
{
    return _hgi;
}

PXR_NAMESPACE_CLOSE_SCOPE
