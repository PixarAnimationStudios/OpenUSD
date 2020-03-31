//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/mesh.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
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

  // Create an empty repr.
  _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                          _ReprComparator(reprToken));
  if (it == _reprs.end()) {
      _reprs.emplace_back(reprToken, HdReprSharedPtr());
  }
}

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

void LoFiMesh::_PopulatePrimvar(HdSceneDelegate* sceneDelegate,
                                HdInterpolation interpolation,
                                LoFiVertexBufferChannelBits channel,
                                const VtValue& value,
                                bool needReallocate)
{
  _vertexArray->SetHaveChannel(channel);
  uint32_t numInputElements = 0;
  uint32_t numOutputElements = _triangles.size();
  const char* datasPtr = NULL;
  switch(channel)
  {
    case CHANNEL_POSITION:
      _positions = value.Get<VtArray<GfVec3f>>();
      numInputElements = _positions.size();
      if(!numInputElements) return;
      datasPtr = (const char*)&_positions.cdata()[0][0];
      break;
    case CHANNEL_NORMAL:
      _normals = value.Get<VtArray<GfVec3f>>();
      numInputElements = _normals.size();
      if(!numInputElements) return;
      datasPtr = (const char*)&_normals.cdata()[0][0];
      break;
    case CHANNEL_COLOR:
      _colors = value.Get<VtArray<GfVec3f>>();
      numInputElements = _colors.size();
      if(!numInputElements) return;
      datasPtr = (const char*)&_colors.cdata()[0][0];
      break;
    case CHANNEL_UVS:
      _uvs = value.Get<VtArray<GfVec2f>>();
      numInputElements = _uvs.size();
      if(!numInputElements) return;
      datasPtr = (const char*)&_uvs.cdata()[0][0];
      break;
    default:
      return;
  }

  if(!_vertexArray->HaveBuffer(channel))
  {
    LoFiVertexBufferSharedPtr buffer = 
      LoFiVertexArray::CreateBuffer(channel, numInputElements,
        numOutputElements);
    _vertexArray->SetBuffer(channel, buffer);
  }

  LoFiVertexBufferSharedPtr buffer =
    _vertexArray->GetBuffer(channel);

  size_t dataHash = 
    buffer->ComputeDatasHash(datasPtr);
    
  if(dataHash != buffer->GetDatasHash())
  {
    buffer->SetDatasHash(dataHash);
    std::cout << "NEW DATA  :( = " << dataHash << std::endl;
  }
  else
  {
    std::cout << "RECYCLED DATA :D = " << dataHash << std::endl;
  }
}

void LoFiMesh::_PopulateMesh( HdSceneDelegate*              sceneDelegate,
                              HdDirtyBits*                  dirtyBits,
                              TfToken const                 &reprToken,
                              LoFiResourceRegistrySharedPtr registry)
{
  _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdMeshReprDesc& desc = descs[0];

  const SdfPath& id = GetId();

  // get triangulated topology
  if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) 
  {
    HdMeshTopology topology = HdMeshTopology(GetMeshTopology(sceneDelegate), 0);

    LoFiTriangulateMesh(
      topology.GetFaceVertexCounts(), 
      topology.GetFaceVertexIndices(),
      _triangles, 
      _samples
    );
  }

  bool needReallocate = (_triangles.size() != _vertexArray->GetNumElements());
  std::cout << "VERTEX ARRAY NEED REALLOCATE : " << needReallocate << std::endl;
  _vertexArray->SetNumElements(_triangles.size());

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
        _primvarSourceMap[pv.name] = {GetPrimvar(sceneDelegate, pv.name), interp};
        VtValue value = _primvarSourceMap[pv.name].data;

        if(pv.name == HdTokens->points)
        {
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_POSITION,
                          value,
                          needReallocate);
        }
        
        else if(pv.name == HdTokens->normals)
        {
          _PopulatePrimvar(sceneDelegate,
                          interp,
                          CHANNEL_NORMAL,
                          value,
                          needReallocate);
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
        /*
          std::cout << "Has Colors" << std::endl;
        //else if(pv.name == HdTokens->primvarsDisplayColor)
        //  std::cout << "Has Normals" << std::endl;
        //else if(pv.name == HdTokens->texC)
        //  std::cout << "Has UVs" << std::endl;

        std::cout << "Primvar : " << pv.name << std::endl;
        std::cout << "Interpolation : " <<  _primvarSourceMap[pv.name].interpolation << std::endl;
        std::cout << "Type : " <<  value.GetTypeName() << std::endl;
        std::cout << "Is Array : " <<  value.IsArrayValued() << std::endl;
        */
      }
    }
  }

  

/*
  // get points
  {
    VtValue value = sceneDelegate->Get(id, HdTokens->points);
    _points = value.Get<VtVec3fArray>();
    std::cout << "DEFORM DIRTY NEED AN UPDATE" << std::endl;
  }
*/
  /*
  if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
      HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->widths) ||
      HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) 
  {
    _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
  }

  if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
    _transform = GfMatrix4f(sceneDelegate->GetTransform(id));
    std::cout << "TRANSFORM DIRTY NEED AN UPDATE" << std::endl;
  }

  if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
    _UpdateVisibility(sceneDelegate, dirtyBits);
  }
  */
}

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

  // check initialized
  bool initialized = false;
  if(_vertexArray.get() != NULL ) initialized = true;

  // get vertex array from registry or create it
  // 
  // LoFiVertexArraySharedPtr vertexArray = 
  //   resourceRegistry->GetVertexArray(instanceId);

  if(!initialized) 
  {
    _instanceId = GetId().GetHash();
    _vertexArray = LoFiVertexArraySharedPtr(new LoFiVertexArray());
    auto instance = resourceRegistry->RegisterVertexArray(_instanceId);
    instance.SetValue(_vertexArray);  
  }
  
  _PopulateMesh(sceneDelegate, dirtyBits, reprToken, resourceRegistry);
  
  
  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
