//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/mesh.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/drawItem.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include <iostream>
#include <memory>

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"


PXR_NAMESPACE_OPEN_SCOPE

LoFiMesh::LoFiMesh(SdfPath const& id, SdfPath const& instancerId)
    : HdMesh(id, instancerId)
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

  /*
  // Create an empty repr.
  _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                          _ReprComparator(reprToken));
  if (it == _reprs.end()) {
      _reprs.emplace_back(reprToken, HdReprSharedPtr());
  }
  */
 _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
  if(it == _reprs.end())
  {
    // add new repr
    _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
    HdReprSharedPtr &repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= (HdChangeTracker::NewRepr
               | HdChangeTracker::DirtyNormals);

    // add draw item
    LoFiDrawItem *drawItem = new LoFiDrawItem(&_sharedData);
    repr->AddDrawItem(drawItem);
  }
}

void LoFiMesh:: InfosLog()
{
  std::cout << "============================================" << std::endl;
  std::cout << "MESH : " << GetId().GetText() << std::endl;
  std::cout << "Num Samples: "<< _samples.size() << std::endl;
  std::cout << "Have Positions: " << 
    _vertexArray->GetHaveChannel(CHANNEL_POSITION) << std::endl;
  if(_vertexArray->GetHaveChannel(CHANNEL_POSITION))
    std::cout << "Num Positions: " << _positions.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have Normals: " << 
    _vertexArray->GetHaveChannel(CHANNEL_NORMAL) << std::endl;
  if(_vertexArray->GetHaveChannel(CHANNEL_NORMAL))
    std::cout << "Num Normals: " << _normals.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have Colors: " << 
    _vertexArray->GetHaveChannel(CHANNEL_COLOR) << std::endl;
  if(_vertexArray->GetHaveChannel(CHANNEL_COLOR))
    std::cout << "Num Colors: " << _colors.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have UVs: " << 
    _vertexArray->GetHaveChannel(CHANNEL_UV) << std::endl;
  if(_vertexArray->GetHaveChannel(CHANNEL_UV))
    std::cout << "Num UVs: " << _uvs.size() << std::endl;
  std::cout << "============================================" << std::endl;
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
  switch(channel)
  {
    case CHANNEL_POSITION:
      _positions = value.Get<VtArray<GfVec3f>>();
      numInputElements = _positions.size();
      if(!numInputElements)valid = false;
      else datasPtr = (const char*)_positions.cdata();
      break;
    case CHANNEL_NORMAL:
      _normals = value.Get<VtArray<GfVec3f>>();
      numInputElements = _normals.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_normals.cdata();
      break;
    case CHANNEL_COLOR:
      _colors = value.Get<VtArray<GfVec3f>>();
      numInputElements = _colors.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_colors.cdata();
      break;
    case CHANNEL_UV:
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
        channel, 
        numInputElements,
        numOutputElements,
        interpolation);

    size_t bufferKey = buffer->ComputeKey(GetId());

    auto instance = registry->RegisterVertexBuffer(bufferKey);

    if(instance.IsFirstInstance())
    //if(!_vertexArray->HasBuffer(channel))
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
        
        TF_STATUS(TfStringPrintf("WE RECYCLE THIS SHIT =  %d", channel));
        return LoFiVertexBufferState::TO_RECYCLE;
      }
        
      else
      {
        old->SetRawInputDatas(datasPtr);
        old->SetNeedUpdate(false);
        old->SetHash(bufferHash);
        _vertexArray->SetBuffer(channel, old);
        TF_STATUS(TfStringPrintf("NEED TO UPDATE THIS SHIT =  %d", channel));
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
    //HdMeshTopology topology = HdMeshTopology(GetMeshTopology(sceneDelegate), 0);
    LoFiTriangulateMesh(
      topology.GetFaceVertexCounts(), 
      topology.GetFaceVertexIndices(),
      _samples
    );

    _vertexArray->SetTopologyPtr((const GfVec3i*)&_samples[0]);
    _vertexArray->SetNumElements(_samples.size());
    _vertexArray->SetNeedReallocate(true);
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
          LoFiVertexBufferState state =
            _PopulatePrimvar(sceneDelegate,
                            interp,
                            CHANNEL_COLOR,
                            value,
                            registry);
          if(state != LoFiVertexBufferState::INVALID)
              haveAuthoredDisplayColor = true;
        }
      }
    }
  }

  // if no authored normals compute smooth vertex normals
  if(!haveAuthoredNormals && (needReallocate || pointPositionsUpdated))
  {
  
    //if(!_vertexArray->GetHaveChannel(CHANNEL_NORMAL))
    //{
    //  LoFiVertexBufferSharedPtr buffer = 
    //    LoFiVertexArray::CreateBuffer(CHANNEL_NORMAL, _positions.size(),
    //      _samples.size());
    //  _vertexArray->SetBuffer(CHANNEL_NORMAL, buffer);
    //  _vertexArray->SetHaveChannel(CHANNEL_NORMAL);
    //}
    //else
    //{
    //  LoFiVertexBufferSharedPtr buffer = _vertexArray->GetBuffer(CHANNEL_NORMAL);
    //  if(needReallocate || buffer->GetNumOutputElements() != _samples.size())
    //  {
    //    buffer->SetNeedReallocate(true);
    //    buffer->SetNumOutputElements(_samples.size());
    //  }
    //}
    //
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
    
    //LoFiVertexBufferSharedPtr buffer = _vertexArray->GetBuffer(CHANNEL_NORMAL);
    //buffer->SetNeedUpdate(true);
    //buffer->SetValid(true);
    //buffer->SetInterpolation(HdInterpolationVertex);
    //buffer->SetRawInputDatas((const char*)_normals.cdata());
    //size_t dataHash = buffer->ComputeDatasHash((const char*)_normals.cdata());
    //  
    //buffer->SetDatasHash(dataHash);
    
  }

/*
  // if no authored colors compute random vertex colors
  if(!haveAuthoredDisplayColor && needReallocate)
  {
    if(!_vertexArray->GetHaveChannel(CHANNEL_COLOR) || needReallocate)
    {
      LoFiVertexBufferSharedPtr buffer = 
        LoFiVertexArray::CreateBuffer(CHANNEL_COLOR, _positions.size(),
          _samples.size());
      _vertexArray->SetBuffer(CHANNEL_COLOR, buffer);
    }

    LoFiComputeVertexColors( _positions, _colors);
    
    LoFiVertexBufferSharedPtr buffer = _vertexArray->GetBuffer(CHANNEL_COLOR);
    buffer->SetNeedReallocate(false);
    buffer->SetValid(true);
    buffer->SetInterpolation(HdInterpolationVertex);
    buffer->SetRawInputDatas((const char*)_colors.cdata());
    size_t dataHash = buffer->ComputeDatasHash((const char*)_colors.cdata());
      
    buffer->SetDatasHash(dataHash);
    buffer->SetNeedUpdate(true);
    
    _vertexArray->SetHaveChannel(CHANNEL_COLOR);
  }
*/
  // update state
  _vertexArray->UpdateState();
  
  
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
  if(_vertexArray.get() != NULL ) initialized = true;

  // create vertex array and store in registry
  if(!initialized) 
  {
    _instanceId = GetId().GetHash();
    _vertexArray = LoFiVertexArraySharedPtr(new LoFiVertexArray());
    auto instance = resourceRegistry->RegisterVertexArray(_instanceId);
    instance.SetValue(_vertexArray);  

    // get associated draw item
    HdReprSharedPtr& repr = _reprs.back().second;
    LoFiDrawItem* drawItem = static_cast<LoFiDrawItem*>(repr->GetDrawItem(0));
    drawItem->SetBufferArrayHash(_instanceId);
    drawItem->SetVertexArray(_vertexArray.get());
  }
  _UpdateVisibility(sceneDelegate, dirtyBits);
  _PopulateMesh(sceneDelegate, dirtyBits, reprToken, resourceRegistry);
  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
