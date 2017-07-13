//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/unitTestDelegate.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/textureResource.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/rotation.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/ptexTexture.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (rotate)
    (scale)
    (translate)
);

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

Hd_UnitTestDelegate::Hd_UnitTestDelegate(HdRenderIndex *parentIndex,
                                         SdfPath const& delegateID)
  : HdSceneDelegate(parentIndex, delegateID)
  , _hasInstancePrimVars(true), _refineLevel(0)
{
}

void
Hd_UnitTestDelegate::SetRefineLevel(int level)
{
    _refineLevel = level;
    TF_FOR_ALL (it, _meshes) {
        GetRenderIndex().GetChangeTracker().MarkRprimDirty(
            it->first, HdChangeTracker::DirtyRefineLevel);
    }
    TF_FOR_ALL (it, _curves) {
        GetRenderIndex().GetChangeTracker().MarkRprimDirty(
            it->first, HdChangeTracker::DirtyRefineLevel);
    }
    TF_FOR_ALL (it, _refineLevels) {
        it->second = level;
    }
}

void
Hd_UnitTestDelegate::AddMesh(SdfPath const &id)
{
    GfMatrix4f transform(1);
    VtVec3fArray points;
    VtIntArray numVerts;
    VtIntArray verts;
    bool guide = false;
    SdfPath instancerId;
    TfToken scheme = PxOsdOpenSubdivTokens->catmullClark;

    AddMesh(id, transform, points, numVerts, verts, guide, instancerId, scheme);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::AddMesh(SdfPath const &id,
                             GfMatrix4f const &transform,
                             VtVec3fArray const &points,
                             VtIntArray const &numVerts,
                             VtIntArray const &verts,
                             bool guide,
                             SdfPath const &instancerId,
                             TfToken const &scheme,
                             TfToken const &orientation,
                             bool doubleSided)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    index.InsertRprim(HdPrimTypeTokens->mesh, this, id, instancerId);

    _meshes[id] = _Mesh(scheme, orientation, transform,
                        points, numVerts, verts, PxOsdSubdivTags(),
                        VtValue(GfVec4f(1)), CONSTANT, guide, doubleSided);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::AddMesh(SdfPath const &id,
                             GfMatrix4f const &transform,
                             VtVec3fArray const &points,
                             VtIntArray const &numVerts,
                             VtIntArray const &verts,
                             PxOsdSubdivTags const &subdivTags,
                             VtValue const &color,
                             Interpolation colorInterpolation,
                             bool guide,
                             SdfPath const &instancerId,
                             TfToken const &scheme,
                             TfToken const &orientation,
                             bool doubleSided)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    index.InsertRprim(HdPrimTypeTokens->mesh, this, id, instancerId);

    _meshes[id] = _Mesh(scheme, orientation, transform,
                        points, numVerts, verts, subdivTags,
                        color, colorInterpolation, guide, doubleSided);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::AddBasisCurves(SdfPath const &id,
                                    VtVec3fArray const &points,
                                    VtIntArray const &curveVertexCounts,
                                    VtVec3fArray const &normals,
                                    TfToken const &basis,
                                    VtValue const &color,
                                    Interpolation colorInterpolation,
                                    VtValue const &width,
                                    Interpolation widthInterpolation,
                                    SdfPath const &instancerId)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    SdfPath shaderId;
    TfMapLookup(_surfaceShaderBindings, id, &shaderId);
    index.InsertRprim(HdPrimTypeTokens->basisCurves, this, id, instancerId);

    _curves[id] = _Curves(points, curveVertexCounts, 
                          normals,
                          basis,
                          color, colorInterpolation,
                          width, widthInterpolation);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::AddPoints(SdfPath const &id,
                               VtVec3fArray const &points,
                               VtValue const &color,
                               Interpolation colorInterpolation,
                               VtValue const &width,
                               Interpolation widthInterpolation,
                               SdfPath const &instancerId)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    SdfPath shaderId;
    TfMapLookup(_surfaceShaderBindings, id, &shaderId);
    index.InsertRprim(HdPrimTypeTokens->points, this, id, instancerId);

    _points[id] = _Points(points,
                          color, colorInterpolation,
                          width, widthInterpolation);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::AddInstancer(SdfPath const &id,
                                  SdfPath const &parentId,
                                  GfMatrix4f const &rootTransform)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    // add instancer
    index.InsertInstancer(this, id, parentId);
    _instancers[id] = _Instancer();
    _instancers[id].rootTransform = rootTransform;

    if (!parentId.IsEmpty()) {
        _instancers[parentId].prototypes.push_back(id);
    }
}

void
Hd_UnitTestDelegate::SetInstancerProperties(SdfPath const &id,
                                            VtIntArray const &prototypeIndex,
                                            VtVec3fArray const &scale,
                                            VtVec4fArray const &rotate,
                                            VtVec3fArray const &translate)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(prototypeIndex.size() == scale.size())   || 
        !TF_VERIFY(prototypeIndex.size() == rotate.size())  ||
        !TF_VERIFY(prototypeIndex.size() == translate.size())) {
        return;
    }

    _instancers[id].scale = scale;
    _instancers[id].rotate = rotate;
    _instancers[id].translate = translate;
    _instancers[id].prototypeIndices = prototypeIndex;
}

void
Hd_UnitTestDelegate::AddSurfaceShader(SdfPath const &id,
                               std::string const &source,
                               HdShaderParamVector const &params)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertSprim(HdPrimTypeTokens->shader, this, id);
    _surfaceShaders[id] = _SurfaceShader(source, params);
}

void
Hd_UnitTestDelegate::AddTexture(SdfPath const& id, 
                                GlfTextureRefPtr const& texture)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertBprim(HdPrimTypeTokens->texture, this, id);
    _textures[id] = _Texture(texture);
}

void
Hd_UnitTestDelegate::HideRprim(SdfPath const& id) 
{
    _hiddenRprims.insert(id);
    GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();
}

void
Hd_UnitTestDelegate::UnhideRprim(SdfPath const& id) 
{
    _hiddenRprims.erase(id);
    GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();
}

void
Hd_UnitTestDelegate::SetReprName(SdfPath const &id, TfToken const &reprName)
{
   if (_meshes.find(id) != _meshes.end()) {
       _meshes[id].reprName = reprName;
   }
}

void
Hd_UnitTestDelegate::SetRefineLevel(SdfPath const &id, int refineLevel)
{
    _refineLevels[id] = refineLevel;
    HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
    tracker.MarkRprimDirty(id, HdChangeTracker::DirtyRefineLevel);
}

static VtVec3fArray _AnimatePositions(VtVec3fArray const &positions, float time)
{
    VtVec3fArray result = positions;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] += GfVec3f((float)(0.5*sin(0.5*i + time)), 0, 0);
    }
    return result;
}

void
Hd_UnitTestDelegate::UpdatePositions(SdfPath const &id, float time)
{
   if (_meshes.find(id) != _meshes.end()) {
       _meshes[id].points = _AnimatePositions(_meshes[id].points, time);
   }
   else if(_curves.find(id) != _curves.end()) {
       _curves[id].points = _AnimatePositions(_curves[id].points, time);
   }
   else if(_points.find(id) != _points.end()) {
       _points[id].points = _AnimatePositions(_points[id].points, time);
   } else {
       return;
   }
   HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
   tracker.MarkRprimDirty(id, HdChangeTracker::DirtyPoints);
}

void
Hd_UnitTestDelegate::UpdateRprims(float time)
{
    // update prims
    float delta = 0.01f;
    HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
    TF_FOR_ALL (it, _meshes) {
        tracker.MarkRprimDirty(it->first, HdChangeTracker::DirtyPrimVar);
        if (it->second.colorInterpolation == CONSTANT) {
            GfVec4f color = it->second.color.Get<GfVec4f>();
            color[0] = fmod(color[0] + delta, 1.0f);
            color[1] = fmod(color[1] + delta*2, 1.0f);
            it->second.color = VtValue(color);
        }
    }
}

void
Hd_UnitTestDelegate::UpdateCurvePrimVarsInterpMode(float time)
{
    // update curve prims to use uniform color
    HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
    TF_FOR_ALL (it, _curves) {        
        if (it->second.colorInterpolation != UNIFORM) {
            tracker.MarkRprimDirty(it->first, HdChangeTracker::DirtyPrimVar);            
            // AddCurves adds two basis curve elements
            GfVec4f colors[] = { GfVec4f(1, 0, 0, 1), GfVec4f(0, 0, 1, 1) };
            VtValue color = VtValue(_BuildArray(&colors[0], 
                                         sizeof(colors)/sizeof(colors[0])));
            it->second.color = VtValue(color);
            it->second.colorInterpolation = UNIFORM;
        }
    }
}

void
Hd_UnitTestDelegate::UpdateInstancerPrimVars(float time)
{
    // update instancers
    TF_FOR_ALL (it, _instancers) {
        for (size_t i = 0; i < it->second.rotate.size(); ++i) {
            GfQuaternion q = GfRotation(GfVec3d(1, 0, 0), i*time).GetQuaternion();
            GfVec4f quat(q.GetReal(),
                         q.GetImaginary()[0],
                         q.GetImaginary()[1],
                         q.GetImaginary()[2]);
            it->second.rotate[i] = quat;
        }

        GetRenderIndex().GetChangeTracker().MarkInstancerDirty(
            it->first,
            HdChangeTracker::DirtyPrimVar);

        // propagate dirtiness to all prototypes
        TF_FOR_ALL (protoIt, it->second.prototypes) {
            if (_instancers.find(*protoIt) != _instancers.end()) continue;
            GetRenderIndex().GetChangeTracker().MarkRprimDirty(
                *protoIt, HdChangeTracker::DirtyInstancer);
        }
    }
}

void
Hd_UnitTestDelegate::UpdateInstancerPrototypes(float time)
{
    // update instancer prototypes
    TF_FOR_ALL (it, _instancers) {
        // rotate prototype indices
        int numInstances = it->second.prototypeIndices.size();
        if (numInstances > 0) {
            int firstPrototype = it->second.prototypeIndices[0];
            for (int i = 1; i < numInstances; ++i) {
                it->second.prototypeIndices[i-1] = it->second.prototypeIndices[i];
            }
            it->second.prototypeIndices[numInstances-1] = firstPrototype;
        }

        // invalidate instance index
        TF_FOR_ALL (protoIt, it->second.prototypes) {
            if (_instancers.find(*protoIt) != _instancers.end()) continue;
            GetRenderIndex().GetChangeTracker().MarkRprimDirty(
                *protoIt, HdChangeTracker::DirtyInstanceIndex);
        }
    }
}

void
Hd_UnitTestDelegate::AddCamera(SdfPath const &id)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertSprim(HdPrimTypeTokens->camera, this, id);
    _cameras[id] = _Camera();
}

void
Hd_UnitTestDelegate::UpdateCamera(SdfPath const &id,
                                  TfToken const &key,
                                  VtValue value)
{
    _cameras[id].params[key] = value;
   HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
   // XXX: we could be more granular here if the tokens weren't in hdx.
   tracker.MarkSprimDirty(id, HdChangeTracker::AllDirty);
}

void
Hd_UnitTestDelegate::UpdateTask(SdfPath const &id,
                                TfToken const &key,
                                VtValue value)
{
    _tasks[id].params[key] = value;

   // Update dirty bits for tokens we recognize.
   HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
   if (key == HdTokens->params) {
       tracker.MarkTaskDirty(id, HdChangeTracker::DirtyParams);
   } else if (key == HdTokens->collection) {
       tracker.MarkTaskDirty(id, HdChangeTracker::DirtyCollection);
   } else if (key == HdTokens->children) {
       tracker.MarkTaskDirty(id, HdChangeTracker::DirtyChildren);
   } else {
       TF_CODING_ERROR("Unknown key %s", key.GetText());
   }
}

/*virtual*/
TfToken
Hd_UnitTestDelegate::GetRenderTag(SdfPath const& id, TfToken const& reprName)
{
    HD_TRACE_FUNCTION();

    if (_hiddenRprims.find(id) != _hiddenRprims.end()) {
        return HdTokens->hidden;
    }

    if (_Mesh *mesh = TfMapLookupPtr(_meshes, id)) {
        if (mesh->guide) {
            return HdTokens->guide;
        } else {
            return HdTokens->geometry;
        }
    } else if (_curves.count(id) > 0) {
        return HdTokens->geometry;
    } else if (_points.count(id) > 0) {
        return HdTokens->geometry;
    }

    return HdTokens->hidden;
}

/*virtual*/
HdMeshTopology 
Hd_UnitTestDelegate::GetMeshTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    HdMeshTopology topology;
    const _Mesh &mesh = _meshes[id];

    return HdMeshTopology(mesh.scheme,
                          mesh.orientation,
                          mesh.numVerts,
                          mesh.verts);
}

/*virtual*/
HdBasisCurvesTopology 
Hd_UnitTestDelegate::GetBasisCurvesTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    const _Curves &curve = _curves[id];

    // Need to implement testing support for basis curves
    return HdBasisCurvesTopology(HdTokens->cubic,
                                 curve.basis,
                                 HdTokens->nonperiodic,
                                 curve.curveVertexCounts,
                                 VtIntArray());
}

/*virtual*/
PxOsdSubdivTags
Hd_UnitTestDelegate::GetSubdivTags(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    const _Mesh &mesh = _meshes[id];

    return mesh.subdivTags;
}

/*virtual*/
GfRange3d
Hd_UnitTestDelegate::GetExtent(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    GfRange3d range;
    VtVec3fArray points;
    if(_meshes.find(id) != _meshes.end()) {
        points = _meshes[id].points; 
    }
    else if(_curves.find(id) != _curves.end()) {
        points = _curves[id].points; 
    }
    else if(_points.find(id) != _points.end()) {
        points = _points[id].points; 
    }
    TF_FOR_ALL(it, points) {
        range.UnionWith(*it);
    }

    return range;
}

/*virtual*/
bool
Hd_UnitTestDelegate::GetDoubleSided(SdfPath const& id)
{
    if (_meshes.find(id) != _meshes.end()) {
        return _meshes[id].doubleSided;
    }
    return false;
}

/*virtual*/
int 
Hd_UnitTestDelegate::GetRefineLevel(SdfPath const& id)
{
    if (_refineLevels.find(id) != _refineLevels.end()) {
        return _refineLevels[id];
    }
    // returns fallback refinelevel
    return _refineLevel;
}

/*virtual*/
VtIntArray
Hd_UnitTestDelegate::GetInstanceIndices(SdfPath const& instancerId,
                                        SdfPath const& prototypeId)
{
    HD_TRACE_FUNCTION();
    VtIntArray indices(0);
    //
    // XXX: this is very naive implementation for unit test.
    //
    //   transpose prototypeIndices/instances to instanceIndices/prototype
    if (_Instancer *instancer = TfMapLookupPtr(_instancers, instancerId)) {
        size_t prototypeIndex = 0;
        for (;prototypeIndex < instancer->prototypes.size(); ++prototypeIndex) {
            if (instancer->prototypes[prototypeIndex] == prototypeId) break;
        }
        if (prototypeIndex == instancer->prototypes.size()) return indices;

        // XXX use const_ptr
        for (size_t i = 0; i < instancer->prototypeIndices.size(); ++i) {
            if (instancer->prototypeIndices[i] == static_cast<int>(prototypeIndex)) {
                indices.push_back(i);
            }
        }
    }
    return indices;
}

/*virtual*/
GfMatrix4d
Hd_UnitTestDelegate::GetInstancerTransform(SdfPath const& instancerId,
                                           SdfPath const& prototypeId)
{
    HD_TRACE_FUNCTION();
    if (_Instancer *instancer = TfMapLookupPtr(_instancers, instancerId)) {
        return GfMatrix4d(instancer->rootTransform);
    }
    return GfMatrix4d(1);
}

/*virtual*/
std::string
Hd_UnitTestDelegate::GetSurfaceShaderSource(SdfPath const &shaderId)
{
    if (_SurfaceShader *shader = TfMapLookupPtr(_surfaceShaders, shaderId)) {
        return shader->source;
    } else {
        return TfToken();
    }
}

/*virtual*/
HdShaderParamVector
Hd_UnitTestDelegate::GetSurfaceShaderParams(SdfPath const &shaderId)
{
    if (_SurfaceShader *shader = TfMapLookupPtr(_surfaceShaders, shaderId)) {
        return shader->params;
    }
    
    return HdShaderParamVector();
}

/*virtual*/
VtValue
Hd_UnitTestDelegate::GetSurfaceShaderParamValue(SdfPath const &shaderId, 
                              TfToken const &paramName)
{
    if (_SurfaceShader *shader = TfMapLookupPtr(_surfaceShaders, shaderId)) {
        TF_FOR_ALL(paramIt, shader->params) {
            if (paramIt->GetName() == paramName)
                return paramIt->GetFallbackValue();
        }
    }
    return VtValue();
}

/*virtual*/
HdTextureResource::ID
Hd_UnitTestDelegate::GetTextureResourceID(SdfPath const& textureId)
{
    return SdfPath::Hash()(textureId);
}

/*virtual*/
HdTextureResourceSharedPtr
Hd_UnitTestDelegate::GetTextureResource(SdfPath const& textureId)
{
    GlfTextureHandleRefPtr texture =
        GlfTextureRegistry::GetInstance().GetTextureHandle(_textures[textureId].texture);

    // Simple way to detect if the glf texture is ptex or not
    bool isPtex = false;
#ifdef PXR_PTEX_SUPPORT_ENABLED
    GlfPtexTextureRefPtr pTex = 
        TfDynamic_cast<GlfPtexTextureRefPtr>(_textures[textureId].texture);
    if (pTex) {
        isPtex = true;
    }
#endif

    return HdTextureResourceSharedPtr(
        new HdSimpleTextureResource(texture, isPtex));
}

/*virtual*/
GfMatrix4d
Hd_UnitTestDelegate::GetTransform(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    if(_meshes.find(id) != _meshes.end()) {
        return GfMatrix4d(_meshes[id].transform);
    }
    return GfMatrix4d(1);
}

/*virtual*/
bool
Hd_UnitTestDelegate::GetVisible(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    return true;
}

/*virtual*/
VtValue
Hd_UnitTestDelegate::Get(SdfPath const& id, TfToken const& key)
{
    HD_TRACE_FUNCTION();

    // camera, light, tasks
    if (_tasks.find(id) != _tasks.end()) {
        return _tasks[id].params[key];
    } else if (_cameras.find(id) != _cameras.end()) {
        return _cameras[id].params[key];
    } else if (_lights.find(id) != _lights.end()) {
        return _lights[id].params[key];
    }

    VtValue value;
    if (key == HdTokens->points) {
        // We could be a mesh or a cuve
        if(_meshes.find(id) != _meshes.end()) {
            return VtValue(_meshes[id].points);
        }
        else if(_curves.find(id) != _curves.end()) {
            return VtValue(_curves[id].points);
        }
        else if(_points.find(id) != _points.end()) {
            return VtValue(_points[id].points);
        }
    } else if (key == HdTokens->normals) {
        if(_curves.find(id) != _curves.end()) {
            return VtValue(_curves[id].normals);
        }
    } else if (key == HdTokens->color) {
        if(_meshes.find(id) != _meshes.end()) {
            return _meshes[id].color;
        }
        else if (_curves.find(id) != _curves.end()) {
            return _curves[id].color;
        }
        else if (_points.find(id) != _points.end()) {
            return _points[id].color;
        }
    } else if (key == HdTokens->widths) {
        if(_curves.find(id) != _curves.end()) {
            return _curves[id].width;
        }
        else if(_points.find(id) != _points.end()) {
            return _points[id].width;
        }
    } else if (key == _tokens->scale) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].scale);
        }
    } else if (key == _tokens->rotate) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].rotate);
        }
    } else if (key == _tokens->translate) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].translate);
        }
    } else if (key == HdShaderTokens->surfaceShader) {
        SdfPath shaderId;
        TfMapLookup(_surfaceShaderBindings, id, &shaderId);

        return VtValue(shaderId);
    }


    return value;
}

/* virtual */
TfToken
Hd_UnitTestDelegate::GetReprName(SdfPath const &id)
{
    HD_TRACE_FUNCTION();

    if (_meshes.find(id) != _meshes.end()) {
        return _meshes[id].reprName;
    }
    return TfToken();
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarVertexNames(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    names.push_back(HdTokens->points);
    if(_meshes.find(id) != _meshes.end()) {
        if (_meshes[id].colorInterpolation == VERTEX) {
            names.push_back(HdTokens->color);
        }
    } else if (_curves.find(id) != _curves.end()) {
        if (_curves[id].colorInterpolation == VERTEX) {
            names.push_back(HdTokens->color);
        }
        if (_curves[id].widthInterpolation == VERTEX) {
            names.push_back(HdTokens->widths);
        }
        if (_curves[id].normals.size() > 0) {
            names.push_back(HdTokens->normals);
        }
    } else if (_points.find(id) != _points.end()) {
        if (_points[id].colorInterpolation == VERTEX) {
            names.push_back(HdTokens->color);
        }
        if (_points[id].widthInterpolation == VERTEX) {
            names.push_back(HdTokens->widths);
        }
    }
    return names;
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarVaryingNames(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    if (_curves.find(id) != _curves.end()) {
        if (_curves[id].widthInterpolation == VARYING) {
            names.push_back(HdTokens->widths);
        }
    }
    return names;
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarFacevaryingNames(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    if (_meshes.find(id) != _meshes.end()) {
        if (_meshes[id].colorInterpolation == FACEVARYING) {
            names.push_back(HdTokens->color);
        }
    }
    return names;
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarUniformNames(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    if(_meshes.find(id) != _meshes.end()) {
        if (_meshes[id].colorInterpolation == UNIFORM) {
            names.push_back(HdTokens->color);
        }
    } else if (_curves.find(id) != _curves.end()) {
        if (_curves[id].colorInterpolation == UNIFORM) {
            names.push_back(HdTokens->color);
        }
        if (_curves[id].widthInterpolation == UNIFORM) {
            names.push_back(HdTokens->widths);
        }
    }
    return names;
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarConstantNames(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    if(_meshes.find(id) != _meshes.end()) {
        if (_meshes[id].colorInterpolation == CONSTANT) {
            names.push_back(HdTokens->color);
        }
    } else if (_curves.find(id) != _curves.end()) {
        if (_curves[id].colorInterpolation == CONSTANT) {
            names.push_back(HdTokens->color);
        }
        if (_curves[id].widthInterpolation == CONSTANT) {
              names.push_back(HdTokens->widths);
        }
    } else if (_points.find(id) != _points.end()) {
        if (_points[id].colorInterpolation == CONSTANT) {
            names.push_back(HdTokens->color);
        }
        if (_points[id].widthInterpolation == CONSTANT) {
              names.push_back(HdTokens->widths);
        }
    }
    return names;
}

/* virtual */
TfTokenVector
Hd_UnitTestDelegate::GetPrimVarInstanceNames(SdfPath const &id)
{
    HD_TRACE_FUNCTION();

    TfTokenVector names;
    if (!_hasInstancePrimVars) return names;
    if (_instancers.find(id) != _instancers.end()) {
        names.push_back(_tokens->scale);
        names.push_back(_tokens->rotate);
        names.push_back(_tokens->translate);
    }
    return names;
}

void
Hd_UnitTestDelegate::AddCube(SdfPath const &id, GfMatrix4f const &transform, bool guide,
                             SdfPath const &instancerId, TfToken const &scheme)
{
    GfVec3f points[] = {
        GfVec3f( 1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f, 1.0f ),
        GfVec3f( 1.0f,-1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f,-1.0f ),
        GfVec3f(-1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f,-1.0f,-1.0f ),
    };

    if (scheme == PxOsdOpenSubdivTokens->loop) {
        int numVerts[] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
        int verts[] = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            0, 6, 5, 0, 5, 1,
            4, 7, 3, 4, 3, 2,
            0, 3, 7, 0, 7, 6,
            4, 2, 1, 4, 1, 5,
        };
        AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            guide,
            instancerId,
            scheme);
    } else {
        int numVerts[] = { 4, 4, 4, 4, 4, 4 };
        int verts[] = {
            0, 1, 2, 3,
            4, 5, 6, 7,
            0, 6, 5, 1,
            4, 7, 3, 2,
            0, 3, 7, 6,
            4, 2, 1, 5,
        };
        AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            guide,
            instancerId,
            scheme);
    }
}

void
Hd_UnitTestDelegate::AddPolygons(
    SdfPath const &id, GfMatrix4f const &transform,
    Hd_UnitTestDelegate::Interpolation colorInterp,
    SdfPath const &instancerId)
{
    int numVerts[] = { 3, 4, 5 };
    int verts[] = {
        0, 1, 2,
        1, 3, 4, 2,
        3, 5, 6, 7, 4
    };
    GfVec3f points[] = {
        GfVec3f(-2.0f,  0.0f, -0.5f ),
        GfVec3f(-1.0f, -1.0f,  0.0f ),
        GfVec3f(-1.0f,  1.0f,  0.0f ),
        GfVec3f( 0.0f, -1.0f,  0.2f ),
        GfVec3f( 0.0f,  1.0f,  0.2f ),
        GfVec3f( 1.0f, -1.0f,  0.0f ),
        GfVec3f( 2.0f,  0.0f, -0.5f ),
        GfVec3f( 1.0f,  1.0f,  0.0f ),
    };


    PxOsdSubdivTags subdivTags;
    VtValue color;

    if (colorInterp == Hd_UnitTestDelegate::CONSTANT) {
        color = VtValue(GfVec4f(1, 1, 0, 1));
    } else if (colorInterp == Hd_UnitTestDelegate::UNIFORM) {
        GfVec4f colors[] = { GfVec4f(1, 0, 0, 1),
                             GfVec4f(0, 0, 1, 1),
                             GfVec4f(0, 1, 0, 1) };
        color = VtValue(_BuildArray(&colors[0], sizeof(colors)/sizeof(colors[0])));
    } else if (colorInterp == Hd_UnitTestDelegate::VERTEX) {
        VtVec4fArray colorArray(sizeof(points)/sizeof(points[0]));
        for (size_t i = 0; i < colorArray.size(); ++i) {
            colorArray[i] = GfVec4f(fabs(sin(0.5*i)),
                                    fabs(cos(0.7*i)),
                                    fabs(sin(0.9*i)*cos(0.25*i)),
                                    1);
        }
        color = VtValue(colorArray);
    } else if (colorInterp == Hd_UnitTestDelegate::FACEVARYING) {
        VtVec4fArray colorArray(sizeof(verts)/sizeof(verts[0]));
        for (size_t i = 0; i < colorArray.size(); ++i) {
            colorArray[i] = GfVec4f(fabs(sin(0.5*i)),
                                    fabs(cos(0.7*i)),
                                    fabs(sin(0.9*i)*cos(0.25*i)),
                                    1);
        }
        color = VtValue(colorArray);
    }

    AddMesh(id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            subdivTags,
            color,
            colorInterp,
            false,
            instancerId);
}

static void
_CreateGrid(int nx, int ny, std::vector<GfVec3f> *points,
            std::vector<int> *numVerts, std::vector<int> *verts,
            GfMatrix4f const &transform)
{
    if (nx == 0 && ny == 0) return;
    // create a unit plane (-1 ~ 1)
    for (int y = 0; y <= ny; ++y) {
        for (int x = 0; x <= nx; ++x) {
            GfVec3f p(2.0*x/float(nx) - 1.0,
                      2.0*y/float(ny) - 1.0,
                      0);
            points->push_back(p);
        }
    }
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            numVerts->push_back(4);
            verts->push_back(    y*(nx+1) + x    );
            verts->push_back(    y*(nx+1) + x + 1);
            verts->push_back((y+1)*(nx+1) + x + 1);
            verts->push_back((y+1)*(nx+1) + x    );
        }
    }
}

void
Hd_UnitTestDelegate::AddGrid(SdfPath const &id, int nx, int ny,
                             GfMatrix4f const &transform,
                             bool rightHanded, bool doubleSided,
                             SdfPath const &instancerId)
{
    std::vector<GfVec3f> points;
    std::vector<int> numVerts;
    std::vector<int> verts;
    _CreateGrid(nx, ny, &points, &numVerts, &verts, transform);

    AddMesh(id,
            transform,
            _BuildArray(&points[0], points.size()),
            _BuildArray(&numVerts[0], numVerts.size()),
            _BuildArray(&verts[0], verts.size()),
            false,
            instancerId,
            PxOsdOpenSubdivTokens->catmark,
            rightHanded ? HdTokens->rightHanded : HdTokens->leftHanded,
            doubleSided);
}

void
Hd_UnitTestDelegate::AddGridWithFaceColor(SdfPath const &id, int nx, int ny,
                                          GfMatrix4f const &transform,
                                          bool rightHanded, bool doubleSided,
                                          SdfPath const &instancerId)
{
    std::vector<GfVec3f> points;
    std::vector<int> numVerts;
    std::vector<int> verts;
    PxOsdSubdivTags subdivTags;
    _CreateGrid(nx, ny, &points, &numVerts, &verts, transform);

    VtVec4fArray colorArray(numVerts.size());
    for (size_t i = 0; i < numVerts.size(); ++i) {
        colorArray[i] = GfVec4f(fabs(sin(0.1*i)),
                                fabs(cos(0.3*i)),
                                fabs(sin(0.7*i)*cos(0.25*i)),
                                1);
    }

    AddMesh(id,
            transform,
            _BuildArray(&points[0], points.size()),
            _BuildArray(&numVerts[0], numVerts.size()),
            _BuildArray(&verts[0], verts.size()),
            subdivTags,
            VtValue(colorArray),
            Hd_UnitTestDelegate::UNIFORM,
            false,
            instancerId,
            PxOsdOpenSubdivTokens->catmark,
            rightHanded ? HdTokens->rightHanded : HdTokens->leftHanded,
            doubleSided);
}

void
Hd_UnitTestDelegate::AddGridWithVertexColor(SdfPath const &id, int nx, int ny,
                                            GfMatrix4f const &transform,
                                            bool rightHanded, bool doubleSided,
                                            SdfPath const &instancerId)
{
    std::vector<GfVec3f> points;
    std::vector<int> numVerts;
    std::vector<int> verts;
    PxOsdSubdivTags subdivTags;
    _CreateGrid(nx, ny, &points, &numVerts, &verts, transform);

    VtVec4fArray colorArray(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        colorArray[i] = GfVec4f(fabs(sin(0.1*i)),
                                fabs(cos(0.3*i)),
                                fabs(sin(0.7*i)*cos(0.25*i)),
                                1);
    }

    AddMesh(id,
            transform,
            _BuildArray(&points[0], points.size()),
            _BuildArray(&numVerts[0], numVerts.size()),
            _BuildArray(&verts[0], verts.size()),
            subdivTags,
            VtValue(colorArray),
            Hd_UnitTestDelegate::VERTEX,
            false,
            instancerId,
            PxOsdOpenSubdivTokens->catmark,
            rightHanded ? HdTokens->rightHanded : HdTokens->leftHanded,
            doubleSided);
}

void
Hd_UnitTestDelegate::AddGridWithFaceVaryingColor(SdfPath const &id, int nx, int ny,
                                                 GfMatrix4f const &transform,
                                                 bool rightHanded, bool doubleSided,
                                                 SdfPath const &instancerId)
{
    std::vector<GfVec3f> points;
    std::vector<int> numVerts;
    std::vector<int> verts;
    PxOsdSubdivTags subdivTags;
    _CreateGrid(nx, ny, &points, &numVerts, &verts, transform);

    VtVec4fArray colorArray(verts.size());
    for (size_t i = 0; i < verts.size(); ++i) {
        colorArray[i] = GfVec4f(fabs(sin(0.1*i)),
                                fabs(cos(0.3*i)),
                                fabs(sin(0.7*i)*cos(0.25*i)),
                                1);
    }

    AddMesh(id,
            transform,
            _BuildArray(&points[0], points.size()),
            _BuildArray(&numVerts[0], numVerts.size()),
            _BuildArray(&verts[0], verts.size()),
            subdivTags,
            VtValue(colorArray),
            Hd_UnitTestDelegate::FACEVARYING,
            false,
            instancerId,
            PxOsdOpenSubdivTokens->catmark,
            rightHanded ? HdTokens->rightHanded : HdTokens->leftHanded,
            doubleSided);
}

void
Hd_UnitTestDelegate::AddCurves(
    SdfPath const &id, TfToken const &basis, GfMatrix4f const &transform,
    Hd_UnitTestDelegate::Interpolation colorInterp,
    Hd_UnitTestDelegate::Interpolation widthInterp,
    bool authoredNormals,
    SdfPath const &instancerId)
{
    int curveVertexCounts[] = { 4, 4 };

    GfVec3f points[] = {
        GfVec3f( 1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f, 1.0f ),
        GfVec3f( 1.0f,-1.0f, 1.0f ),

        GfVec3f(-1.0f,-1.0f,-1.0f ),
        GfVec3f(-1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f,-1.0f,-1.0f ),
    };

    GfVec3f normals[] = {
        GfVec3f(   .0f,-1.0f,  .0f ),
        GfVec3f(   .0f,  .0f, 1.0f ),
        GfVec3f(  1.0f,  .0f,  .0f ),
        GfVec3f(  1.0f,  .0f,  .0f ),

        GfVec3f(   .0f,  .0f, 1.0f ),
        GfVec3f(   .0f,  .0f, 1.0f ),
        GfVec3f( -1.0f,  .0f,  .0f ),
        GfVec3f( -1.0f,  .0f,  .0f ),
    };

    VtVec3fArray authNormals;
    if (authoredNormals)
        authNormals = _BuildArray(normals, sizeof(normals)/sizeof(normals[0]));

    for(size_t i = 0;i < sizeof(points) / sizeof(points[0]); ++ i) {
        GfVec4f tmpPoint = GfVec4f(points[i][0], points[i][1], points[i][2], 1.0f);
        tmpPoint = tmpPoint * transform;
        points[i] = GfVec3f(tmpPoint[0], tmpPoint[1], tmpPoint[2]);
    }

    VtValue color;
    if (colorInterp == Hd_UnitTestDelegate::CONSTANT) {
        color = VtValue(GfVec4f(1));
    } else if (colorInterp == Hd_UnitTestDelegate::UNIFORM) {
        GfVec4f colors[] = { GfVec4f(1, 0, 0, 1), GfVec4f(0, 0, 1, 1) };
        color = VtValue(_BuildArray(&colors[0], sizeof(colors)/sizeof(colors[0])));
    } else if (colorInterp == Hd_UnitTestDelegate::VERTEX) {
        GfVec4f colors[] = { GfVec4f(0, 0, 1, 1),
                             GfVec4f(0, 1, 0, 1),
                             GfVec4f(0, 1, 1, 1),
                             GfVec4f(1, 0, 0, 1),
                             GfVec4f(1, 0, 1, 1),
                             GfVec4f(1, 1, 0, 1),
                             GfVec4f(1, 1, 1, 1),
                             GfVec4f(0.5, 0.5, 1, 1) };
        color = VtValue(_BuildArray(&colors[0], sizeof(colors)/sizeof(colors[0])));
    }

    VtValue width;
    if (widthInterp == Hd_UnitTestDelegate::CONSTANT) {
        width = VtValue(0.1f);
    } else if (widthInterp == Hd_UnitTestDelegate::UNIFORM) {
        float widths[] = { 0.1f, 0.4f };
        width = VtValue(_BuildArray(&widths[0], sizeof(widths)/sizeof(widths[0])));
    } else if (widthInterp == Hd_UnitTestDelegate::VERTEX) {
        float widths[] = { 0, 0.1f, 0.2f, 0.3f, 0.1f, 0.2f, 0.2f, 0.1f };
        width = VtValue(_BuildArray(&widths[0], sizeof(widths)/sizeof(widths[0])));
    } else if (widthInterp == Hd_UnitTestDelegate::VARYING) {
        float widths[] = { 0, 0.1f, 0.2f, 0.3f};
        width = VtValue(_BuildArray(&widths[0], sizeof(widths)/sizeof(widths[0])));
    }

    AddBasisCurves(
        id,
        _BuildArray(points, sizeof(points)/sizeof(points[0])),
        _BuildArray(curveVertexCounts,
                    sizeof(curveVertexCounts)/sizeof(curveVertexCounts[0])),
        authNormals,
        basis,
        color, colorInterp,
        width, widthInterp,
        instancerId);
}

void
Hd_UnitTestDelegate::AddPoints(
    SdfPath const &id, GfMatrix4f const &transform,
    Hd_UnitTestDelegate::Interpolation colorInterp,
    Hd_UnitTestDelegate::Interpolation widthInterp,
    SdfPath const &instancerId)
{
    int numPoints = 500;

    VtVec3fArray points(numPoints);
    float s = 0, t = 0;
    for (int i = 0; i < numPoints; ++i) {
        GfVec4f p (sin(s)*cos(t), sin(s)*sin(t), cos(s), 1);
        p = p * transform;
        points[i] = GfVec3f(p[0], p[1], p[2]);;
        s += 0.10;
        t += 0.34;
    }

    VtValue color;
    if (colorInterp == Hd_UnitTestDelegate::CONSTANT ||
        colorInterp == Hd_UnitTestDelegate::UNIFORM) {
        color = VtValue(GfVec4f(1, 1, 1, 1));
    } else if (colorInterp == Hd_UnitTestDelegate::VERTEX) {
        VtVec4fArray colors(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            colors[i] = GfVec4f(fabs(sin(0.1*i)),
                                fabs(cos(0.3*i)),
                                fabs(sin(0.7*i)*cos(0.25*i)),
                                1);
        }
        color = VtValue(colors);
    }

    VtValue width;
    if (widthInterp == Hd_UnitTestDelegate::CONSTANT ||
        widthInterp == Hd_UnitTestDelegate::UNIFORM) {
        width = VtValue(0.1f);
    } else { // VERTEX
        VtFloatArray widths(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            widths[i] = 0.1*fabs(sin(0.1*i));
        }
        width = VtValue(widths);
    }

    AddPoints(
        id, points,
        color, colorInterp,
        width, widthInterp,
        instancerId);
}

void
Hd_UnitTestDelegate::AddSubdiv(SdfPath const &id, GfMatrix4f const &transform,
                               SdfPath const &instancerId)
{
/*

     0-----3-------4-----7
     |     ||      |     |
     |     || hole |     |
     |     ||       \    |
     1-----2--------[5]--6
           |        /    |
           |       |     |
           |       |     |
           8-------9----10

       =  : creased edge
       [] : corner vertex

*/
    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = {
        0, 1, 2, 3,
        3, 2, 5, 4,
        4, 5, 6, 7,
        2, 8, 9, 5,
        5, 9, 10, 6,
    };
    GfVec3f points[] = {
        GfVec3f(-1.0f, 0.0f,  1.0f ),
        GfVec3f(-1.0f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  1.0f ),
        GfVec3f( 0.0f, 0.0f,  1.0f ),
        GfVec3f( 0.5f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  1.0f ),
        GfVec3f(-0.5f, 0.0f, -1.0f ),
        GfVec3f( 0.0f, 0.0f, -1.0f ),
        GfVec3f( 1.0f, 0.0f, -1.0f ),
    };
    int holes[] = { 1 };
    int creaseLengths[] = { 2 };
    int creaseIndices[] = { 2, 3 };
    float creaseSharpnesses[] = { 5.0f };
    int cornerIndices[] = { 5 };
    float cornerSharpnesses[] = { 5.0f };

    PxOsdSubdivTags subdivTags;
    subdivTags.SetHoleIndices(_BuildArray(holes,
        sizeof(holes)/sizeof(holes[0])));
    subdivTags.SetCreaseLengths(_BuildArray(creaseLengths,
        sizeof(creaseLengths)/sizeof(creaseLengths[0])));
    subdivTags.SetCreaseIndices(_BuildArray(creaseIndices,
        sizeof(creaseIndices)/sizeof(creaseIndices[0])));
    subdivTags.SetCreaseWeights(_BuildArray(creaseSharpnesses,
        sizeof(creaseSharpnesses)/sizeof(creaseSharpnesses[0])));
    subdivTags.SetCornerIndices(_BuildArray(cornerIndices,
        sizeof(cornerIndices)/sizeof(cornerIndices[0])));
    subdivTags.SetCornerWeights(_BuildArray(cornerSharpnesses,
        sizeof(cornerSharpnesses)/sizeof(cornerSharpnesses[0])));

    subdivTags.SetVertexInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);
    subdivTags.SetFaceVaryingInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);

    AddMesh(id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            subdivTags,
            /*color=*/VtValue(GfVec4f(1)),
            /*colorInterpolation=*/Hd_UnitTestDelegate::CONSTANT,
            false,
            instancerId);
}

void
Hd_UnitTestDelegate::Remove(SdfPath const &id)
{
    GetRenderIndex().RemoveRprim(id);
}

void
Hd_UnitTestDelegate::Clear()
{
    GetRenderIndex().Clear();
}

void
Hd_UnitTestDelegate::MarkRprimDirty(SdfPath path, HdDirtyBits flag)
{
    GetRenderIndex().GetChangeTracker().MarkRprimDirty(path, flag);
}

GfVec3f
Hd_UnitTestDelegate::PopulateBasicTestSet()
{
    GfMatrix4d dmat;
    double xPos = 0.0;

    // grids
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddGrid(SdfPath("/grid1"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(xPos,  0.0, 0.0));
        AddGridWithFaceColor(SdfPath("/grid2"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(xPos,  3.0, 0.0));
        AddGridWithVertexColor(SdfPath("/grid3"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(xPos,  6.0, 0.0));
        AddGridWithFaceVaryingColor(SdfPath("/grid3a"), 3, 3, GfMatrix4f(dmat));

        xPos += 3.0;
    }

    // non-quads
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddPolygons(SdfPath("/nonquads1"), GfMatrix4f(dmat),
                             Hd_UnitTestDelegate::CONSTANT);

        dmat.SetTranslate(GfVec3d(xPos,  0.0, 0.0));
        AddPolygons(SdfPath("/nonquads2"), GfMatrix4f(dmat),
                             Hd_UnitTestDelegate::UNIFORM);

        dmat.SetTranslate(GfVec3d(xPos,  3.0, 0.0));
        AddPolygons(SdfPath("/nonquads3"), GfMatrix4f(dmat),
                             Hd_UnitTestDelegate::VERTEX);

        dmat.SetTranslate(GfVec3d(xPos,  6.0, 0.0));
        AddPolygons(SdfPath("/nonquads4"), GfMatrix4f(dmat),
                             Hd_UnitTestDelegate::FACEVARYING);

        xPos += 3.0;
    }

    // more grids (backface, single sided)
    {
        // rotate X 180
        dmat.SetRotate(GfRotation(GfVec3d(1, 0, 0), 180.0));
        dmat.SetTranslateOnly(GfVec3d(xPos, -3.0, 0.0));
        AddGrid(SdfPath("/grid4"), 10, 10, GfMatrix4f(dmat));

        // inverse X
        dmat.SetScale(GfVec3d(-1, 1, 1));
        dmat.SetTranslateOnly(GfVec3d(xPos, 0.0, 0.0));
        AddGridWithFaceColor(SdfPath("/grid5"), 10, 10,
                                      GfMatrix4f(dmat));

        // inverse Z
        dmat.SetScale(GfVec3d(1, 1, -1));
        dmat.SetTranslateOnly(GfVec3d(xPos, 3.0, 0.0));
        AddGridWithVertexColor(SdfPath("/grid6"), 10, 10,
                                      GfMatrix4f(dmat));

        // left-handed
        dmat.SetTranslate(GfVec3d(xPos, 6.0, 0.0));
        AddGridWithFaceVaryingColor(SdfPath("/grid7"), 3, 3,
                                             GfMatrix4f(dmat),
                                             /*rightHanded=*/false);

        xPos += 3.0;
    }

    // more grids (backface, double sided)
    {
        // rotate X 180
        dmat.SetRotate(GfRotation(GfVec3d(1, 0, 0), 180.0));
        dmat.SetTranslateOnly(GfVec3d(xPos, -3.0, 0.0));
        AddGrid(SdfPath("/grid8"), 10, 10, GfMatrix4f(dmat),
                         /*rightHanded=*/true, /*doubleSided=*/true);

        // inverse X
        dmat.SetScale(GfVec3d(-1, 1, 1));
        dmat.SetTranslateOnly(GfVec3d(xPos, 0.0, 0.0));
        AddGridWithFaceColor(SdfPath("/grid9"), 10, 10,
                                      GfMatrix4f(dmat),
                                      /*rightHanded=*/true,
                                      /*doubleSided=*/true);

        // inverse Z
        dmat.SetScale(GfVec3d(1, 1, -1));
        dmat.SetTranslateOnly(GfVec3d(xPos, 3.0, 0.0));
        AddGridWithVertexColor(SdfPath("/grid10"), 10, 10,
                                      GfMatrix4f(dmat),
                                      /*rightHanded=*/true,
                                      /*doubleSided=*/true);

        // left-handed
        dmat.SetTranslate(GfVec3d(xPos, 6.0, 0.0));
        AddGridWithFaceVaryingColor(SdfPath("/grid11"), 3, 3,
                                             GfMatrix4f(dmat),
                                             /*righthanded=*/false,
                                             /*doubleSided=*/true);

        xPos += 3.0;
    }

    // cubes
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddCube(SdfPath("/cube1"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->loop);

        dmat.SetTranslate(GfVec3d(xPos, 0.0, 0.0));
        AddCube(SdfPath("/cube2"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->catmark);

        dmat.SetTranslate(GfVec3d(xPos, 3.0, 0.0));
        AddCube(SdfPath("/cube3"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->bilinear);

        xPos += 3.0;
    }

    // cubes with authored reprs
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddCube(SdfPath("/cube4"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->catmark);
        SetReprName(SdfPath("/cube4"), HdTokens->smoothHull);

        dmat.SetTranslate(GfVec3d(xPos, 0.0, 0.0));
        AddCube(SdfPath("/cube5"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->catmark);
        SetReprName(SdfPath("/cube5"), HdTokens->hull);

        dmat.SetTranslate(GfVec3d(xPos, 3.0, 0.0));
        AddCube(SdfPath("/cube6"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->catmark);
        SetReprName(SdfPath("/cube6"), HdTokens->refined);
        SetRefineLevel(SdfPath("/cube6"), std::max(1, _refineLevel));

        dmat.SetTranslate(GfVec3d(xPos, 6.0, 0.0));
        AddCube(SdfPath("/cube7"), GfMatrix4f(dmat), false, SdfPath(),
                         PxOsdOpenSubdivTokens->catmark);
        SetReprName(SdfPath("/cube7"), HdTokens->wireOnSurf);

        xPos += 3.0;
    }

    // curves
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddCurves(SdfPath("/curve1"), HdTokens->linear, GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX);

        dmat.SetTranslate(GfVec3d(xPos, 0.0, 0.0));
        AddCurves(SdfPath("/curve2"), HdTokens->bezier, GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX);

        dmat.SetTranslate(GfVec3d(xPos, 3.0, 0.0));
        AddCurves(SdfPath("/curve3"), HdTokens->bSpline, GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT);

        dmat.SetTranslate(GfVec3d(xPos, 6.0, 0.0));
        AddCurves(SdfPath("/curve4"), HdTokens->catmullRom, GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT);

        xPos += 3.0;
    }

    // points
    {
        dmat.SetTranslate(GfVec3d(xPos, -3.0, 0.0));
        AddPoints(SdfPath("/points1"), GfMatrix4f(dmat),
                       Hd_UnitTestDelegate::CONSTANT, Hd_UnitTestDelegate::CONSTANT);

        dmat.SetTranslate(GfVec3d(xPos, 0.0, 0.0));
        AddPoints(SdfPath("/points2"), GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT);

        dmat.SetTranslate(GfVec3d(xPos, 3.0, 0.0));
        AddPoints(SdfPath("/points3"), GfMatrix4f(dmat),
                           Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX);
    }

    return GfVec3f(xPos/2.0, 0, 0);
}

GfVec3f
Hd_UnitTestDelegate::PopulateInvalidPrimsSet()
{
    // empty mesh
    AddGrid(SdfPath("/empty_mesh"), 0, 0, GfMatrix4f(1));

    // empty curve
    AddBasisCurves(SdfPath("/empty_curve"),
                            VtVec3fArray(),
                            VtIntArray(),
                            VtVec3fArray(),
                            HdTokens->linear,
                            VtValue(GfVec4f(1)), Hd_UnitTestDelegate::CONSTANT,
                            VtValue(1.0f), Hd_UnitTestDelegate::CONSTANT);

    // empty point
    AddPoints(SdfPath("/empty_points"),
                       VtVec3fArray(),
                       VtValue(GfVec4f(1)), Hd_UnitTestDelegate::CONSTANT,
                       VtValue(1.0f), Hd_UnitTestDelegate::CONSTANT);

    return GfVec3f(0);
}

PXR_NAMESPACE_CLOSE_SCOPE

