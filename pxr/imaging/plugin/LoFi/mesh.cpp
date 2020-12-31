//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/plugin/LoFi/mesh.h"
#include "pxr/imaging/plugin/LoFi/instancer.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/drawItem.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"

#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include <iostream>
#include <memory>

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"


PXR_NAMESPACE_OPEN_SCOPE

LoFiMesh::LoFiMesh(SdfPath const& id)
  : HdMesh(id)
  , _vertexArray(nullptr)
{
}

HdDirtyBits
LoFiMesh::GetInitialDirtyBitsMask() const
{
  // The initial dirty bits control what data is available on the first
  // run through _PopulateLoFiMesh(), so it should list every data item
  // that _PopulateLoFi requests.
  int mask = HdChangeTracker::Clean
            | HdChangeTracker::InitRepr
            | HdChangeTracker::DirtyExtent
            | HdChangeTracker::DirtyNormals
            | HdChangeTracker::DirtyPoints
            | HdChangeTracker::DirtyPrimvar
            | HdChangeTracker::DirtyRepr
            | HdChangeTracker::DirtyTopology
            | HdChangeTracker::DirtyTransform
            | HdChangeTracker::DirtyVisibility
            ;

  return (HdDirtyBits)mask;
}

HdDirtyBits
LoFiMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

//PxOsdOpenSubdivTokens->none
void 
LoFiMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
  TF_UNUSED(dirtyBits);

 _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
  if(it == _reprs.end())
  {
    // add new repr
    _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
    HdReprSharedPtr &repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= (HdChangeTracker::NewRepr);

    // add draw items
    HdRepr::DrawItemUniquePtr drawItem = std::make_unique<LoFiDrawItem>(&_sharedData);
    repr->AddDrawItem(std::move(drawItem));

  }
}

LoFiVertexBufferState
LoFiMesh::_PopulatePrimvar( HdSceneDelegate* sceneDelegate,
                            HdInterpolation interpolation,
                            LoFiAttributeChannel channel,
                            const VtValue& value,
                            LoFiResourceRegistrySharedPtr registry)
{
  uint32_t numInputElements = 0;
  uint32_t numOutputElements = _samples.size();
  const char* datasPtr = NULL;
  bool valid = true;
  short tuppleSize = 3;
  std::string name = GetId().GetText();
  switch(channel)
  {
    case CHANNEL_POSITION:
      name += "_POSITION";
      _positions = value.Get<VtArray<GfVec3f>>();
      numInputElements = _positions.size();
      if(!numInputElements)valid = false;
      else datasPtr = (const char*)_positions.cdata();
      break;
    case CHANNEL_NORMAL:
      name += "_NORMAL";
      _normals = value.Get<VtArray<GfVec3f>>();
      numInputElements = _normals.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_normals.cdata();
      break;
    case CHANNEL_COLOR:
      name += "_COLOR";
      _colors = value.Get<VtArray<GfVec3f>>();
      numInputElements = _colors.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_colors.cdata();
      break;
    case CHANNEL_UV:
      name += "_UVS";
      _uvs = value.Get<VtArray<GfVec2f>>();
      numInputElements = _uvs.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_uvs.cdata();
      break;
    default:
      return LoFiVertexBufferState::INVALID;
  }

  if(valid)
  {
    _vertexArray->SetHaveChannel(channel);

    LoFiVertexBufferSharedPtr buffer = 
      LoFiVertexArray::CreateBuffer(
        _vertexArray->GetTopology(),
        channel, 
        numInputElements,
        numOutputElements,
        interpolation,
        name);

    size_t bufferKey = buffer->ComputeKey(GetId());

    auto instance = registry->RegisterVertexBuffer(bufferKey);

    if(instance.IsFirstInstance())
    {
      instance.SetValue(buffer); 
      buffer = instance.GetValue();
      _vertexArray->SetBuffer(channel, buffer);
      buffer->SetNeedReallocate(true);
      buffer->SetValid(valid);
      buffer->SetRawInputDatas(datasPtr);
      buffer->SetNeedUpdate(true);
      return LoFiVertexBufferState::TO_REALLOCATE;
    }
    else 
    {
      size_t bufferHash = buffer->ComputeHash(datasPtr);
      LoFiVertexBufferSharedPtr old = instance.GetValue();
      _vertexArray->SetBuffer(channel, old);

      if(bufferHash == old->GetHash()) 
      {
        return LoFiVertexBufferState::TO_RECYCLE;
      }
        
      else
      {
        old->SetRawInputDatas(datasPtr);
        old->SetNeedUpdate(true);
        old->SetHash(bufferHash);
        _vertexArray->SetBuffer(channel, old);
        return LoFiVertexBufferState::TO_UPDATE;
      }
    }
  }
  else return LoFiVertexBufferState::INVALID;
}

void LoFiMesh::_PopulateMesh( HdSceneDelegate*              sceneDelegate,
                              HdDirtyBits*                  dirtyBits,
                              TfToken const                 &reprToken,
                              LoFiResourceRegistrySharedPtr registry)
{
  _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdMeshReprDesc& desc = descs[0];

  const SdfPath& id = GetId();

  HdMeshTopology topology = HdMeshTopology(GetMeshTopology(sceneDelegate), 0);

  bool needReallocate = false;
  // get triangulated topology
  if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) 
  {
    LoFiTriangulateMesh(
      topology.GetFaceVertexCounts(), 
      topology.GetFaceVertexIndices(),
      _samples
    );

    LoFiTopology* topo = _vertexArray->GetTopology();
    topo->samples = (const int*)&_samples[0];
    topo->numElements = _samples.size()/3;
    _vertexArray->SetNumElements(_samples.size());
    _vertexArray->SetNeedUpdate(true);
    
    needReallocate = true;
  }

  if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) 
  {
    GfMatrix4d transform = sceneDelegate->GetTransform(id);
    _sharedData.bounds.SetMatrix(transform);
  }

  if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) 
  {
    _sharedData.bounds.SetRange(GetExtent(sceneDelegate));
  }
  
  bool pointPositionsUpdated = false;
  bool haveAuthoredNormals = false;
  bool haveAuthoredDisplayColor = false;

  // get primvars
  HdPrimvarDescriptorVector primvars;
  for (size_t i=0; i < HdInterpolationCount; ++i) 
  {
    HdInterpolation interp = static_cast<HdInterpolation>(i);
    primvars = GetPrimvarDescriptors(sceneDelegate, interp);
    for (HdPrimvarDescriptor const& pv: primvars) 
    {
      if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv.name)) 
      {
        VtValue value = GetPrimvar(sceneDelegate, pv.name);
        if(pv.name == HdTokens->points)
        {
          LoFiVertexBufferState state =
            _PopulatePrimvar( sceneDelegate,
                              interp,
                              CHANNEL_POSITION,
                              value,
                              registry);
          if(state != LoFiVertexBufferState::TO_RECYCLE && 
            state != LoFiVertexBufferState::INVALID)
              pointPositionsUpdated = true;
        }
        
        else if(pv.name == HdTokens->normals)
        {
          LoFiVertexBufferState state =
            _PopulatePrimvar(sceneDelegate,
                            interp,
                            CHANNEL_NORMAL,
                            value,
                            registry);
          if(state != LoFiVertexBufferState::INVALID)
              haveAuthoredNormals = true;
        }
          
        else if(pv.name == TfToken("uv") || pv.name == TfToken("st"))
        {
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_UV,
                          value,
                          registry);
        }
        
        else if(pv.name == TfToken("displayColor") || 
          pv.name == TfToken("primvars:displayColor"))
        {
          if(interp != HdInterpolationConstant)
          {
             LoFiVertexBufferState state =
            _PopulatePrimvar(sceneDelegate,
                            interp,
                            CHANNEL_COLOR,
                            value,
                            registry);
            if(state != LoFiVertexBufferState::INVALID)
                haveAuthoredDisplayColor = true;
            _varyingColor = true;
          }
          else 
          {
            if(value.IsHolding<VtArray<GfVec3f>>())
              _displayColor = value.UncheckedGet<VtArray<GfVec3f>>()[0];
            else if(value.IsHolding<GfVec3f>())
              _displayColor = value.UncheckedGet<GfVec3f>();
            else 
              _displayColor = GfVec3f(1.f,0.75f,0.25f);
            _varyingColor = false;
          }
        }
      }
    }
  }
  // if no authored normals compute smooth vertex normals
  if(!haveAuthoredNormals && (needReallocate || pointPositionsUpdated))
  {
    LoFiComputeVertexNormals( _positions,
                              topology.GetFaceVertexCounts(),
                              topology.GetFaceVertexIndices(),
                              _samples,
                              _normals);
    
    _PopulatePrimvar( sceneDelegate,
                      HdInterpolationVertex,
                      CHANNEL_NORMAL,
                      VtValue(_normals),
                      registry);
  }

  // if no authored colors compute random vertex colors (test)
  //if(!haveAuthoredDisplayColor)
  //{
  //  LoFiComputeVertexColors( _positions, _colors);
//
  //  _PopulatePrimvar( sceneDelegate,
  //                    HdInterpolationVertex,
  //                    CHANNEL_COLOR,
  //                    VtValue(_colors),
  //                    registry);
  //}

  // update state
  _vertexArray->UpdateState();

}

void 
LoFiMesh::_PopulateBinder(LoFiResourceRegistrySharedPtr registry)
{
  // get associated draw item
  HdReprSharedPtr& repr = _reprs.back().second;

  // surface
  {
    LoFiDrawItem* drawItem = static_cast<LoFiDrawItem*>(repr->GetDrawItem(0));

    LoFiBinder* binder = drawItem->Binder();
    binder->Clear();
    binder->CreateUniformBinding(LoFiUniformTokens->model, LoFiGLTokens->mat4, 0);
    binder->CreateUniformBinding(LoFiUniformTokens->view, LoFiGLTokens->mat4, 1);
    binder->CreateUniformBinding(LoFiUniformTokens->projection, LoFiGLTokens->mat4, 2);
    binder->CreateUniformBinding(LoFiUniformTokens->normalMatrix, LoFiGLTokens->mat4, 3);
    binder->CreateUniformBinding(LoFiUniformTokens->viewport, LoFiGLTokens->vec4, 4);
    binder->CreateUniformBinding(LoFiUniformTokens->displayColor, LoFiGLTokens->vec3, 5);

    binder->CreateAttributeBinding(LoFiBufferTokens->position, LoFiGLTokens->vec3, CHANNEL_POSITION);
    binder->CreateAttributeBinding(LoFiBufferTokens->normal, LoFiGLTokens->vec3, CHANNEL_NORMAL);
    if(_colors.size())
      binder->CreateAttributeBinding(LoFiBufferTokens->color, LoFiGLTokens->vec3, CHANNEL_COLOR);
    if(_uvs.size())
      binder->CreateAttributeBinding(LoFiBufferTokens->uv, LoFiGLTokens->vec2, CHANNEL_UV);

    binder->SetProgramType(LOFI_PROGRAM_MESH);
    binder->ComputeProgramName();
  }
}

//_sharedData.materialTag = _GetMaterialTag(delegate->GetRenderIndex());

void
LoFiMesh::Sync( HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits,
                TfToken const   &reprToken)
{
  HD_TRACE_FUNCTION();
  HF_MALLOC_TAG_FUNCTION();
  // get render index
  HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();

  // get resource registry
  LoFiResourceRegistrySharedPtr resourceRegistry =
    std::static_pointer_cast<LoFiResourceRegistry>(
      renderIndex.GetResourceRegistry());

  uint64_t meshId = (uint64_t)this;
  
  // check initialized
  bool initialized = false;

  if(_vertexArray.get() != NULL) initialized = true;
  
  // get associated draw item
  HdReprSharedPtr& repr = _reprs.back().second;
  LoFiDrawItem* drawItem = static_cast<LoFiDrawItem*>(repr->GetDrawItem(0));

  // create vertex array and store in registry
  if(!initialized) 
  {
    size_t surfaceId = GetId().GetHash();
    // surface
    {
      _vertexArray = LoFiVertexArraySharedPtr(new LoFiVertexArray(LoFiTopology::Type::TRIANGLES));
      auto surfaceInstance = resourceRegistry->RegisterVertexArray(surfaceId);
      surfaceInstance.SetValue(_vertexArray);  

      drawItem->SetBufferArrayHash(surfaceId);
      drawItem->SetVertexArray(_vertexArray.get());
    }
  }
  
  _UpdateVisibility(sceneDelegate, dirtyBits);
  if(IsVisible())
  {
    _PopulateMesh(sceneDelegate, dirtyBits, reprToken, resourceRegistry);
    // instances
    if (! GetInstancerId().IsEmpty())
    {
      // Retrieve instance transforms from the instancer.
      HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

      LoFiInstancer* instancer = 
        static_cast<LoFiInstancer*>(renderIndex.GetInstancer(GetInstancerId()));
      VtMatrix4dArray transforms =
        instancer->ComputeInstanceTransforms(GetId());
      drawItem->PopulateInstancesXforms(transforms);
      drawItem->PopulateInstancesColors(instancer->GetColors());
    }
    else drawItem->ClearInstancesXforms();
    drawItem->SetDisplayColor(_displayColor);
  }

  if(!initialized)
  {
    _PopulateBinder(resourceRegistry);
  }
  
  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
