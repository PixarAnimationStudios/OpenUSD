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
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include <iostream>

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
            | HdChangeTracker::DirtyPoints
            | HdChangeTracker::DirtyTopology
            | HdChangeTracker::DirtyTransform
            | HdChangeTracker::DirtyVisibility
            | HdChangeTracker::DirtyPrimvar
            | HdChangeTracker::DirtyNormals
            | HdChangeTracker::DirtyInstancer
            ;

  return (HdDirtyBits)mask;
}

HdDirtyBits
LoFiMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

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
    _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
    HdReprSharedPtr &repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= HdChangeTracker::NewRepr;

    // add draw item
    LoFiDrawItem *drawItem = new LoFiDrawItem(&_sharedData);
    repr->AddDrawItem(drawItem);
  }
}

/*
void
HdStMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        // set dirty bit to say we need to sync a new repr (buffer array
        // ranges may change)
        *dirtyBits |= HdChangeTracker::NewRepr;

        _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);

        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdMeshReprDesc &desc = descs[descIdx];

            size_t numDrawItems = _GetNumDrawItemsForDesc(desc);
            if (numDrawItems == 0) continue;

            for (size_t itemId = 0; itemId < numDrawItems; itemId++) {
                HdDrawItem *drawItem = new HdStDrawItem(&_sharedData);
                repr->AddDrawItem(drawItem);
                HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();

                switch (desc.geomStyle) {
                case HdMeshGeomStyleHull:
                case HdMeshGeomStyleHullEdgeOnly:
                case HdMeshGeomStyleHullEdgeOnSurf:
                {
                    drawingCoord->SetTopologyIndex(HdStMesh::HullTopology);
                    if (!(_customDirtyBitsInUse & DirtyHullIndices)) {
                        _customDirtyBitsInUse |= DirtyHullIndices;
                        *dirtyBits |= DirtyHullIndices;
                    }
                    break;
                }

                case HdMeshGeomStylePoints:
                {
                    // in the current implementation, we use topology
                    // for points too, to draw a subset of vertex primvars
                    // (note that the points may be followed by the refined
                    // vertices)
                    drawingCoord->SetTopologyIndex(HdStMesh::PointsTopology);
                    if (!(_customDirtyBitsInUse & DirtyPointsIndices)) {
                        _customDirtyBitsInUse |= DirtyPointsIndices;
                        *dirtyBits |= DirtyPointsIndices;
                    }
                    break;
                }

                default:
                {
                    if (!(_customDirtyBitsInUse & DirtyIndices)) {
                        _customDirtyBitsInUse |= DirtyIndices;
                        *dirtyBits |= DirtyIndices;
                    }
                }
                }

                // Set up drawing coord instance primvars.
                drawingCoord->SetInstancePrimvarBaseIndex(
                    HdStMesh::InstancePrimvar);
            } // for each draw item

            if (desc.flatShadingEnabled) {
                if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                    _customDirtyBitsInUse |= DirtyFlatNormals;
                    *dirtyBits |= DirtyFlatNormals;
                }
            } else {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        } // for each repr desc for the repr
    } // if new repr
}
*/
/*
void
LoFiMesh::_UpdatePrimvarSources(HdSceneDelegate* sceneDelegate,
                                HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    SdfPath const& id = GetId();

    HdPrimvarDescriptorVector primvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) 
    {
      HdInterpolation interp = static_cast<HdInterpolation>(i);
      primvars = GetPrimvarDescriptors(sceneDelegate, interp);
      for (HdPrimvarDescriptor const& pv: primvars) 
      {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name) &&
          pv.name != HdTokens->points) 
          {
            std::cout << "FOUND PRIM VAR : " << pv.name << " ===> ";
            switch(interp)
            {
              case HdInterpolationConstant:
                std::cout << "CONSTANT"<<std::endl;
                break;
              case HdInterpolationUniform:
                std::cout << "UNIFORM"<<std::endl;
                break;
              case HdInterpolationVarying:
                std::cout << "VARYING"<<std::endl;
                break;
              case HdInterpolationVertex:
                std::cout << "VERTEX"<<std::endl;
                break;
              case HdInterpolationFaceVarying:
                std::cout << "FACEVARYING"<<std::endl;
                break;
              case HdInterpolationInstance:
                std::cout << "INSTANCE"<<std::endl;
                break;
            }
          _primvarSourceMap[pv.name] = {
              GetPrimvar(sceneDelegate, pv.name),
              interp
          };
        }
      }
    }
}

TfTokenVector
LoFiMesh::_UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
                                            HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    
    SdfPath const& id = GetId();

    // Get all the dirty computed primvars
    HdExtComputationPrimvarDescriptorVector dirtyCompPrimvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) {
        HdExtComputationPrimvarDescriptorVector compPrimvars;
        HdInterpolation interp = static_cast<HdInterpolation>(i);
        compPrimvars = sceneDelegate->GetExtComputationPrimvarDescriptors
                                    (GetId(),interp);

        for (auto const& pv: compPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                dirtyCompPrimvars.emplace_back(pv);
            }
        }
    }

    if (dirtyCompPrimvars.empty()) {
        return TfTokenVector();
    }
    
    HdExtComputationUtils::ValueStore valueStore
        = HdExtComputationUtils::GetComputedPrimvarValues(
            dirtyCompPrimvars, sceneDelegate);

    TfTokenVector compPrimvarNames;
    // Update local primvar map and track the ones that were computed
    for (auto const& compPrimvar : dirtyCompPrimvars) {
        auto const it = valueStore.find(compPrimvar.name);
        if (!TF_VERIFY(it != valueStore.end())) {
            continue;
        }
        
        compPrimvarNames.emplace_back(compPrimvar.name);
        if (compPrimvar.name == HdTokens->points) {
            _points = it->second.Get<VtVec3fArray>();
            //_normalsValid = false;
        } else {
            _primvarSourceMap[compPrimvar.name] = {it->second,
                                                compPrimvar.interpolation};
        }
    }

    return compPrimvarNames;
}
*/

void LoFiMesh:: InfosLog()
{
  std::cout << "============================================" << std::endl;
  std::cout << "MESH : " << GetId().GetText() << std::endl;
  std::cout << "Num Samples: "<< _samples.size() << std::endl;
  std::cout << "Have Positions: " << 
    _vertexArray->HaveChannel(CHANNEL_POSITION) << std::endl;
  if(_vertexArray->HaveChannel(CHANNEL_POSITION))
    std::cout << "Num Positions: " << _positions.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have Normals: " << 
    _vertexArray->HaveChannel(CHANNEL_NORMAL) << std::endl;
  if(_vertexArray->HaveChannel(CHANNEL_NORMAL))
    std::cout << "Num Normals: " << _normals.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have Colors: " << 
    _vertexArray->HaveChannel(CHANNEL_COLOR) << std::endl;
  if(_vertexArray->HaveChannel(CHANNEL_COLOR))
    std::cout << "Num Colors: " << _colors.size() << std::endl;
  std::cout << "--------------------------------------------" << std::endl;
  std::cout << "Have UVs: " << 
    _vertexArray->HaveChannel(CHANNEL_UVS) << std::endl;
  if(_vertexArray->HaveChannel(CHANNEL_UVS))
    std::cout << "Num UVs: " << _uvs.size() << std::endl;
  std::cout << "============================================" << std::endl;
}

LoFiVertexBufferState
LoFiMesh::_PopulatePrimvar(HdSceneDelegate* sceneDelegate,
                                HdInterpolation interpolation,
                                LoFiVertexBufferChannel channel,
                                const VtValue& value,
                                bool needReallocate)
{
  _vertexArray->SetHaveChannel(channel);
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
    case CHANNEL_UVS:
      _uvs = value.Get<VtArray<GfVec2f>>();
      numInputElements = _uvs.size();
      if(!numInputElements) valid = false;
      else datasPtr = (const char*)_uvs.cdata();
      break;
    default:
      return LoFiVertexBufferState::INVALID;
  }

  if(!_vertexArray->HaveBuffer(channel) || needReallocate)
  {
    LoFiVertexBufferSharedPtr buffer = 
      LoFiVertexArray::CreateBuffer(channel, numInputElements,
        numOutputElements);
    _vertexArray->SetBuffer(channel, buffer);
  }

  LoFiVertexBufferSharedPtr buffer = _vertexArray->GetBuffer(channel);
  buffer->SetNeedReallocate(needReallocate);
  buffer->SetValid(valid);
  buffer->SetInterpolation(interpolation);
  buffer->SetRawInputDatas(datasPtr);
  if(valid)
  {
    size_t dataHash = buffer->ComputeDatasHash(datasPtr);
    
    if(dataHash != buffer->GetDatasHash())
    {
      buffer->SetDatasHash(dataHash);
      buffer->SetNeedUpdate(true);
      if(needReallocate) return LoFiVertexBufferState::TO_REALLOCATE;
      else return LoFiVertexBufferState::TO_UPDATE;
    }
    else 
    {
      buffer->SetNeedUpdate(false);
      return LoFiVertexBufferState::TO_RECYCLE;
    }
  }
  
  return LoFiVertexBufferState::INVALID;
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
  }

  if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) 
  {
    GfMatrix4d transform = sceneDelegate->GetTransform(id);
    _sharedData.bounds.SetMatrix(transform); // for CPU frustum culling
  }

  if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) 
  {
    _sharedData.bounds.SetRange(GetExtent(sceneDelegate));
  }

  bool needReallocate = (_samples.size() != _vertexArray->GetNumElements());
  _vertexArray->SetNumElements(_samples.size());
  bool pointPositionsUpdated = false;
  bool haveAuthoredNormals = false;
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
                              needReallocate );
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
                            needReallocate);
          if(state != LoFiVertexBufferState::INVALID)
              haveAuthoredNormals = true;
        }
          
        else if(pv.name == TfToken("uv") || pv.name == TfToken("st"))
        {
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_UVS,
                          value,
                          needReallocate);
        }
        
        else if(pv.name == TfToken("displayColor"))
        {
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_COLOR,
                          value,
                          needReallocate);
        }
      }
    }
  }
  
  
  // if no authored normals compute smooth vertex normals
  if(!haveAuthoredNormals && (needReallocate || pointPositionsUpdated))
  {
    if(!_vertexArray->HaveBuffer(CHANNEL_NORMAL) || needReallocate)
    {
      LoFiVertexBufferSharedPtr buffer = 
        LoFiVertexArray::CreateBuffer(CHANNEL_NORMAL, _positions.size(),
          _samples.size());
      _vertexArray->SetBuffer(CHANNEL_NORMAL, buffer);
    }

    LoFiComputeVertexNormals( _positions,
                              topology.GetFaceVertexCounts(),
                              topology.GetFaceVertexIndices(),
                              _samples,
                              _normals);
    
    LoFiVertexBufferSharedPtr buffer = _vertexArray->GetBuffer(CHANNEL_NORMAL);
    buffer->SetNeedReallocate(false);
    buffer->SetNeedUpdate(true);
    buffer->SetValid(true);
    buffer->SetInterpolation(HdInterpolationVertex);
    buffer->SetRawInputDatas((const char*)_normals.cdata());
    size_t dataHash = buffer->ComputeDatasHash((const char*)_normals.cdata());
      
    buffer->SetDatasHash(dataHash);
    buffer->SetNeedUpdate(true);
    
    _vertexArray->SetHaveChannel(CHANNEL_NORMAL);
  }

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
    boost::static_pointer_cast<LoFiResourceRegistry>(
      renderIndex.GetResourceRegistry());

  uint64_t meshId = (uint64_t)this;

  // register mesh
  if(resourceRegistry->GetMesh(meshId) == NULL)
  {
    HdInstance<LoFiMesh*> instance = resourceRegistry->RegisterMesh(meshId);
    if(instance.IsFirstInstance())instance.SetValue(this);
  }
  
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

  _PopulateMesh(sceneDelegate, dirtyBits, reprToken, resourceRegistry);
  
  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
