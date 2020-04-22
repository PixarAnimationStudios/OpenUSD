//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/points.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/drawItem.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include <iostream>
#include <memory>

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"


PXR_NAMESPACE_OPEN_SCOPE

LoFiPoints::LoFiPoints(SdfPath const& id, SdfPath const& instancerId)
    : HdPoints(id, instancerId)
{
  std::cout << "CREATE LOFI POINT PRIMITIVE :D" << std::endl;
  _topology.type = LoFiTopology::Type::POINTS;
}

HdDirtyBits
LoFiPoints::GetInitialDirtyBitsMask() const
{
  // The initial dirty bits control what data is available on the first
  // run through _PopulateLoFiPoints(), so it should list every data item
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
LoFiPoints::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
LoFiPoints::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
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

void LoFiPoints:: InfosLog()
{
  std::cout << "============================================" << std::endl;
  std::cout << "POINTS : " << GetId().GetText() << std::endl;
  std::cout << "Num Points: "<< _points.size() << std::endl;
  std::cout << "Have Positions: " << 
    _vertexArray->GetHaveChannel(CHANNEL_POSITION) << std::endl;
  if(_vertexArray->GetHaveChannel(CHANNEL_POSITION))
    std::cout << "Num Positions: " << _points.size() << std::endl;
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
  std::cout << "============================================" << std::endl;
}

LoFiVertexBufferState
LoFiPoints::_PopulatePrimvar( HdSceneDelegate* sceneDelegate,
                            HdInterpolation interpolation,
                            LoFiAttributeChannel channel,
                            const VtValue& value,
                            LoFiResourceRegistrySharedPtr registry)
{
  uint32_t numInputElements = 0;
  
  const char* datasPtr = NULL;
  bool valid = true;
  short tuppleSize = 3;
  switch(channel)
  {
    case CHANNEL_POSITION:
      _points = value.Get<VtArray<GfVec3f>>();
      numInputElements = _points.size();
      tuppleSize = 3;
      if(!numInputElements)valid = false;
      else datasPtr = (const char*)_points.cdata();
      break;
    case CHANNEL_WIDTH:
      _widths = value.Get<VtArray<float>>();
      numInputElements = _widths.size();
      tuppleSize = 1;
      for(int i=0;i<_widths.size();++i)_widths[i] = 6;
      if(!numInputElements)valid = false;
      else datasPtr = (const char*)_widths.cdata();
      break;
    case CHANNEL_NORMAL:
      _normals = value.Get<VtArray<GfVec3f>>();
      numInputElements = _normals.size();
      tuppleSize = 3;
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_normals.cdata();
      break;
    case CHANNEL_COLOR:
      _colors = value.Get<VtArray<GfVec3f>>();
      numInputElements = _colors.size();
      tuppleSize = 3;
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_colors.cdata();
      break;
    case CHANNEL_UV:
      _uvs = value.Get<VtArray<GfVec2f>>();
      numInputElements = _uvs.size();
      tuppleSize = 2;
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
        numInputElements,
        tuppleSize,
        interpolation);

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
        old->SetNeedUpdate(false);
        old->SetHash(bufferHash);
        _vertexArray->SetBuffer(channel, old);
        return LoFiVertexBufferState::TO_UPDATE;
      }

    }
  }
  else return LoFiVertexBufferState::INVALID;
}

void LoFiPoints::_PopulatePoints( HdSceneDelegate*              sceneDelegate,
                                  HdDirtyBits*                  dirtyBits,
                                  TfToken const                 &reprToken,
                                  LoFiResourceRegistrySharedPtr registry)
{
  _PointsReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdPointsReprDesc& desc = descs[0];

  const SdfPath& id = GetId();
  bool needReallocate = false;
  

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

  HdInterpolation interp = HdInterpolationVertex;
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

      else if(pv.name == HdTokens->widths)
      {
        LoFiVertexBufferState state =
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_WIDTH,
                          value,
                          registry);
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

  size_t numPoints = _points.size();
  _samples.resize(numPoints);
  for(int i=0;i<numPoints;++i)_samples[i] = i;

  _topology.samples = (const int*)&_samples[0];
  _topology.numElements = numPoints;
  _vertexArray->SetNumElements(numPoints);
  _vertexArray->SetNeedReallocate(true);

  // update state
  _vertexArray->UpdateState();
  
  
}

void 
LoFiPoints::_PopulateBinder(LoFiResourceRegistrySharedPtr registry)
{
  // get associated draw item
  HdReprSharedPtr& repr = _reprs.back().second;
  LoFiDrawItem* drawItem = static_cast<LoFiDrawItem*>(repr->GetDrawItem(0));

  // get binder
  LoFiBinder* binder = drawItem->Binder();
  binder->Clear();
  binder->CreateUniformBinding(LoFiUniformTokens->model, LoFiGLTokens->mat4, 0);
  binder->CreateUniformBinding(LoFiUniformTokens->view, LoFiGLTokens->mat4, 1);
  binder->CreateUniformBinding(LoFiUniformTokens->projection, LoFiGLTokens->mat4, 2);

  binder->CreateAttributeBinding(LoFiBufferTokens->position, LoFiGLTokens->vec3, CHANNEL_POSITION);
  binder->CreateAttributeBinding(LoFiBufferTokens->width, LoFiGLTokens->_float, CHANNEL_WIDTH);
  binder->CreateAttributeBinding(LoFiBufferTokens->normal, LoFiGLTokens->vec3, CHANNEL_NORMAL);
  
  if(_colors.size())
    binder->CreateAttributeBinding(LoFiBufferTokens->color, LoFiGLTokens->vec3, CHANNEL_COLOR);

  binder->SetProgramType(LOFI_PROGRAM_POINT);
  binder->ComputeProgramName();
}

//_sharedData.materialTag = _GetMaterialTag(delegate->GetRenderIndex());

void
LoFiPoints::Sync( HdSceneDelegate *sceneDelegate,
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

  // get associated draw item
  HdReprSharedPtr& repr = _reprs.back().second;
  LoFiDrawItem* drawItem = static_cast<LoFiDrawItem*>(repr->GetDrawItem(0));

  // create vertex array and store in registry
  if(!initialized) 
  {
    _instanceId = GetId().GetHash();
    _vertexArray = LoFiVertexArraySharedPtr(new LoFiVertexArray());
    _vertexArray->SetTopologyPtr(&_topology);
    auto instance = resourceRegistry->RegisterVertexArray(_instanceId);
    instance.SetValue(_vertexArray);  

    drawItem->SetBufferArrayHash(_instanceId);
    drawItem->SetVertexArray(_vertexArray.get());
  }
  _UpdateVisibility(sceneDelegate, dirtyBits);
  _PopulatePoints(sceneDelegate, dirtyBits, reprToken, resourceRegistry);

  // populate binder
  if(!initialized)
  {
    _PopulateBinder(resourceRegistry);
  }

  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
